// Standard includes
#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>

// Local includes
#include "version.hpp"

constexpr std::string_view proper_usage = {
    "\n"
    "   xorc --help\n"
    "   xorc --version\n"
    "   xorc <input_file> <output_file> [--pad=<path_to_pad>] [--pos="
    "<position_in_pad>]\n"
    "   xorc --stdin <output_file> [--pad=<path_to_pad>] "
    "[--pos=<position_in_pad>]\n"
    "   xorc --stdout <input_file> [--pad=<path_to_pad>] "
    "[--pos=<position_in_pad>]\n"
    "   xorc --stdin --stdout [--pad=<path_to_pad>] "
    "[--pos=<position_in_pad>]\n\n"
};

int failure(const std::string_view& msg) {
    std::cout << msg;
    std::cout << proper_usage;
    return 1;
}

int success() {
    return 0;
}

int success(const std::string_view& msg) {
    std::cout << msg;
    return success();
}

class file_open_error : public std::exception {
    const char* err_;

  public:
    explicit file_open_error(const char* err) : err_(err) {}
    [[nodiscard]] const char* what() const noexcept override {
        return this->err_;
    }
};

enum class eof {
    file,
    stdin_, // NOLINT - added underscore to avoid conflicting macros
};

class eof_error : public std::exception {
    eof eof_;

  public:
    explicit eof_error(eof eof) : eof_(eof) {}
    [[nodiscard]] eof get_eof_type() const noexcept { return this->eof_; }
    [[nodiscard]] const char* what() const noexcept override {
        return "Encountered EOF while reading stream.";
    }
};

class file_not_open_error : public std::exception {
    const char* err_;

  public:
    explicit file_not_open_error(const char* err) : err_(err) {}
    [[nodiscard]] const char* what() const noexcept override {
        return this->err_;
    }
};

class istream {
    std::ifstream stream_;
    size_t size_ = 0;

  public:
    void open_file(const std::string_view& name, const size_t pos) {
        if (this->stream_.is_open()) {
            this->close();
        }
        this->stream_.open(
            name.data(),
            std::ios_base::openmode::_S_in | std::ios_base::openmode::_S_bin
                | std::ios_base::openmode::_S_ate);
        if (! this->stream_.is_open()) {
            throw file_open_error("Failed to open file for reading.\n");
        }
        this->size_ = this->stream_.tellg();
        // This conversion is error-prone.
        this->stream_.seekg(static_cast<std::streamoff>(pos));
    }

    void close() { this->stream_.close(); }

    istream() = default;
    istream(const std::string_view& name, const size_t pos) {
        this->open_file(name, pos);
    }
    istream(const istream&) = delete;
    istream(istream&&) noexcept = default;
    istream& operator=(const istream&) = delete;
    istream& operator=(istream&&) noexcept = default;
    ~istream() { this->close(); }

    [[nodiscard]] size_t size() const {
        if (! this->stream_.is_open()) {
            throw file_not_open_error("Cannot get size of stdin.\n");
        }
        return this->size_;
    }

    u_char get() {
        u_char chr = 0;
        if (this->stream_.is_open()) {
            chr = this->stream_.get();
            if (this->stream_.eof()) {
                throw eof_error(eof::file);
            }
            return chr;
        }

        if (! (std::cin >> chr)) {
            throw eof_error(eof::stdin_);
        }
        return chr;
    }
};

class ostream {
    std::ofstream stream_;

  public:
    void open(const std::string_view& name) {
        if (this->stream_.is_open()) {
            this->close();
        }
        this->stream_.open(
            name.data(),
            std::ios_base::openmode::_S_out | std::ios_base::openmode::_S_bin);
        if (! this->stream_.is_open()) {
            throw file_open_error("Failed to open file for writing.\n");
        }
    }

    void close() { this->stream_.close(); }

    ostream() = default;
    explicit ostream(const std::string_view& name) { this->open(name); }
    ostream(const ostream&) = delete;
    ostream(ostream&&) noexcept = default;
    ostream& operator=(const ostream&) = delete;
    ostream& operator=(ostream&&) noexcept = default;
    ~ostream() { this->close(); }

    void put(const u_char chr) {
        if (this->stream_.is_open()) {
            this->stream_.put(static_cast<std::ofstream::char_type>(chr));
        }
        else {
            std::cout << chr;
        }
    }
};

enum class opt_id {
    none,
    help,
    version,
    stdin_,  // NOLINT - added underscore to avoid conflicting macros
    stdout_, // NOLINT - added underscore to avoid conflicting macros
    pad,
    pos,
};

struct opt_t {
    std::string_view name;
    opt_id id = opt_id::none;
    bool value = false;
};

// NOLINTNEXTLINE(readability-magic-numbers)
using opt_arr_t_f = std::array<opt_t, 6>;
constexpr opt_arr_t_f option = {
    {
     { "--help", opt_id::help },
     { "--version", opt_id::version },
     { "--stdin", opt_id::stdin_ },
     { "--stdout", opt_id::stdout_ },
     { "--pad=", opt_id::pad, true },
     { "--pos=", opt_id::pos, true },
     }
};

struct pad_t {
    std::string_view path;
    size_t pos = 0;
};

struct crypt_t {
    std::string_view input = "data";
    pad_t pad = { .path = "pad.key", .pos = 0 };
    std::string_view output = "data.crypt";
    bool use_stdin = false;
    bool use_stdout = false;
};

class show_help : public std::exception {
  public:
    [[nodiscard]] const char* what() const noexcept override {
        return proper_usage.data();
    }
};

class show_version : public std::exception {
  public:
    [[nodiscard]] const char* what() const noexcept override {
        return xor_crypt::compiletime_version;
    }
};

crypt_t get_crypt_args(const std::span<char*>& in_cmd_arg) {
    crypt_t crypt{};
    constexpr opt_t default_opt{};
    opt_t found_opt = default_opt;
    size_t cmd_arg_i = 0;
    bool use_input_file = false;
    bool use_output_file = false;

    for (std::string_view arg : in_cmd_arg) {
        if (arg.rfind("--", 0) != 0) {
            // positional argument
            switch (cmd_arg_i) {
                case 0:
                    if (! crypt.use_stdin) {
                        crypt.input = arg;
                        use_input_file = true;
                        break;
                    }
                    [[fallthrough]];
                case 1:
                    crypt.output = arg;
                    use_output_file = true;
                    break;
                default:
                    throw std::runtime_error(
                        "Error: Too many positional arguments.\n");
            }

            ++cmd_arg_i;
            continue;
        }

        // option
        for (const auto& opt : option) {
            if (opt.value) {
                if (arg.rfind(opt.name) == 0) {
                    found_opt = opt;
                    arg = arg.substr(
                        opt.name.size(), arg.size() - opt.name.size());
                }
            }
            else {
                if (arg == opt.name) {
                    found_opt = opt;
                }
            }
        }

        switch (found_opt.id) {
            case opt_id::none:
                throw std::runtime_error("Error: Invalid option.\n");
            case opt_id::help: throw show_help();
            case opt_id::version: throw show_version();
            case opt_id::stdin_: crypt.use_stdin = true; break;
            case opt_id::stdout_: crypt.use_stdout = true; break;
            case opt_id::pad:
                crypt.pad.path = arg;
                found_opt = default_opt;
                continue;
            case opt_id::pos:
                // NOLINTNEXTLINE(readability-magic-numbers)
                crypt.pad.pos = strtoul(arg.data(), nullptr, 10);
                found_opt = default_opt;
                continue;
            default: throw std::runtime_error("Error: Invalid option id.\n");
        }
    }

    if (crypt.use_stdin && use_input_file) {
        throw std::runtime_error(
            "Error: Cannot receive input from both stdin and a file.\n");
    }

    if (crypt.use_stdout && use_output_file) {
        throw std::runtime_error(
            "Error: Cannot send output to both stdout and a file.\n");
    }

    return crypt;
}

int main(int argc, char** argv) {
    std::span arg{ argv, static_cast<size_t>(argc) };
    if (argc < 2) {
        return failure("Error: No argument provided.\n");
    }

    try {
        crypt_t crypt = get_crypt_args(arg.subspan(1, arg.size() - 1));

        istream input{};
        if (! crypt.use_stdin) {
            input = istream{ crypt.input, 0 };
        }

        istream pad(crypt.pad.path, crypt.pad.pos);

        ostream output{};
        if (! crypt.use_stdout) {
            output = ostream{ crypt.output };
        }

        // TODO: Ensure that the pad size (when also accounting for the
        // position) does not exceed the size of the input.

        // if ((pad.size() -
        //     throw std::runtime_error(
        //         "The pad file is too small for the given pad position "
        //         "and input file.\n");
        // }

        for (size_t i = 0; i < pad.size(); ++i) {
            output.put(input.get() ^ pad.get());
        }
    }
    catch (const std::runtime_error& e) {
        return failure(e.what());
    }
    catch (const file_open_error& e) {
        return failure(e.what());
    }
    catch (const eof_error& e) {
        switch (e.get_eof_type()) {
            case eof::file: return failure(e.what());
            case eof::stdin_: return success();
            default:
                return failure("Invalid eof enum from eof_error exception.\n");
        }
    }
    catch (const show_help& e) {
        return success(e.what());
    }
    catch (const show_version& e) {
        return success(e.what());
    }

    return success();
}

namespace thing {}
