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
    "<position_in_pad>]\n\n"
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

class file_r {
    std::ifstream stream_;
    size_t size_ = 0;

    void close_() { this->stream_.close(); }

  public:
    void open(const std::string_view& name, const size_t pos) {
        if (this->stream_.is_open()) {
            this->close_();
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

    file_r(const std::string_view& name, const size_t pos) {
        this->open(name, pos);
    }
    file_r(const file_r&) = delete;
    file_r(file_r&&) noexcept = default;
    file_r& operator=(const file_r&) = delete;
    file_r& operator=(file_r&&) noexcept = default;
    ~file_r() { this->close_(); }

    [[nodiscard]] size_t size() const { return this->size_; }

    u_char get() { return this->stream_.get(); }
};

class file_w {
    std::ofstream stream_;

    void close_() { this->stream_.close(); }

  public:
    void open(const std::string_view& name) {
        if (this->stream_.is_open()) {
            this->close_();
        }
        this->stream_.open(
            name.data(),
            std::ios_base::openmode::_S_out | std::ios_base::openmode::_S_bin);
        if (! this->stream_.is_open()) {
            throw file_open_error("Failed to open file for writing.\n");
        }
    }

    file_w() = default;
    explicit file_w(const std::string_view& name) { this->open(name); }
    file_w(const file_w&) = delete;
    file_w(file_w&&) noexcept = default;
    file_w& operator=(const file_w&) = delete;
    file_w& operator=(file_w&&) noexcept = default;
    ~file_w() { this->close_(); }

    void put(const u_char chr) {
        this->stream_.put(static_cast<std::ofstream::char_type>(chr));
    }
};

enum class opt_id {
    none,
    help,
    version,
    pad,
    pos,
};

struct opt_t {
    std::string_view name;
    opt_id id = opt_id::none;
    bool value = true;
};

using opt_arr_t_f = std::array<opt_t, 4>;
constexpr opt_arr_t_f option = {
    {
     { "--help", opt_id::help, false },
     { "--version", opt_id::version, false },
     { "--pad=", opt_id::pad },
     { "--pos=", opt_id::pos },
     }
};

struct pad_t {
    std::string_view path;
    size_t pos = 0;
};

struct crypt_t {
    std::string_view input;
    pad_t pad;
    std::string_view output;
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
    crypt_t crypt = {
        .input = "data",
        .pad = {.path = "pad.key", .pos = 0},
        .output = "data.crypt"
    };
    constexpr opt_t default_opt = { .name = "",
                                    .id = opt_id::none,
                                    .value = false };
    opt_t found_opt = default_opt;
    size_t cmd_arg_i = 0;

    for (std::string_view arg : in_cmd_arg) {
        if (arg.rfind("--", 0) != 0) {
            // positional argument
            switch (cmd_arg_i) {
                case 0: crypt.input = arg; break;
                case 1: crypt.output = arg; break;
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

    return crypt;
}

int main(int argc, char** argv) {
    std::span arg{ argv, static_cast<size_t>(argc) };
    if (argc < 2) {
        return failure("Error: No argument provided.\n");
    }

    try {
        crypt_t crypt = get_crypt_args(arg.subspan(1, arg.size() - 1));

        file_r input(crypt.input, 0);
        file_r pad(crypt.pad.path, crypt.pad.pos);
        file_w output(crypt.output);

        if ((pad.size() - crypt.pad.pos) < input.size()) {
            throw std::runtime_error(
                "The pad file is too small for the given pad position "
                "and input file.\n");
        }

        for (size_t i = 0; i < input.size(); ++i) {
            output.put(input.get() ^ pad.get());
        }
    }
    catch (const std::runtime_error& e) {
        return failure(e.what());
    }
    catch (const file_open_error& e) {
        return failure(e.what());
    }
    catch (const show_help& e) {
        return success(e.what());
    }
    catch (const show_version& e) {
        return success(e.what());
    }

    return success();
}
