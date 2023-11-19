// Standard includes
#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

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

using openmode = std::ios_base::openmode;

class file_r {
    std::ifstream stream_;
    std::streamoff size_ = 0;

    void close_() { this->stream_.close(); }

  public:
    void open(const std::string_view& path, const std::streamoff& offset) {
        if (this->stream_.is_open()) {
            this->close_();
        }
        this->stream_.open(
            path.data(), openmode::_S_in | openmode::_S_bin | openmode::_S_ate);
        if (! this->stream_.is_open()) {
            throw std::runtime_error("Failed to open file for reading.\n");
        }
        this->size_ = this->stream_.tellg();
        this->stream_.seekg(offset);
    }

    file_r() = default;
    file_r(const std::string_view& path, const std::streamoff& offset) {
        this->open(path, offset);
    }
    file_r(const file_r&) = delete;
    file_r(file_r&&) noexcept = default;
    file_r& operator=(const file_r&) = delete;
    file_r& operator=(file_r&&) noexcept = default;
    ~file_r() { this->close_(); }

    [[nodiscard]] std::streamoff size() const { return this->size_; }

    char get() {
        char chr = 0;
        this->stream_.get(chr);
        return chr;
    }
};

class file_w {
    std::ofstream stream_;

    void close_() { this->stream_.close(); }

  public:
    void open(const std::string_view& path) {
        if (this->stream_.is_open()) {
            this->close_();
        }
        this->stream_.open(path.data(), openmode::_S_out | openmode::_S_bin);
        if (! this->stream_.is_open()) {
            throw std::runtime_error("Failed to open file for writing.\n");
        }
    }

    file_w() = default;
    explicit file_w(const std::string_view& path) { this->open(path); }
    file_w(const file_w&) = delete;
    file_w(file_w&&) noexcept = default;
    file_w& operator=(const file_w&) = delete;
    file_w& operator=(file_w&&) noexcept = default;
    ~file_w() { this->close_(); }

    void put(const char chr) { this->stream_.put(chr); }
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
    bool value = false;
};

// NOLINTNEXTLINE(readability-magic-numbers)
using opt_arr_t_f = std::array<opt_t, 6>;
constexpr opt_arr_t_f option = {
    {
     { "--help", opt_id::help },
     { "--version", opt_id::version },
     { "--pad=", opt_id::pad, true },
     { "--pos=", opt_id::pos, true },
     }
};

struct pad_t {
    std::string_view path;
    std::streamoff pos = 0;
};

struct crypt_t {
    std::string_view input = "data";
    pad_t pad = { .path = "pad.key", .pos = 0 };
    std::string_view output = "data.crypt";
    bool show_help = false;
    bool show_version = false;
};

crypt_t get_crypt_args(const std::span<char*>& in_cmd_arg) {
    crypt_t crypt{};
    constexpr opt_t default_opt{};
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
            case opt_id::help: crypt.show_help = true; return crypt;
            case opt_id::version: crypt.show_version = true; return crypt;
            case opt_id::pad:
                crypt.pad.path = arg;
                found_opt = default_opt;
                continue;
            case opt_id::pos:
                // NOLINTNEXTLINE(readability-magic-numbers)
                crypt.pad.pos = strtol(arg.data(), nullptr, 10);
                if (crypt.pad.pos < 0) {
                    throw std::runtime_error(
                        "Error: Pad position cannot be negative.\n");
                }
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
        if (crypt.show_help) {
            return success(proper_usage);
        }
        if (crypt.show_version) {
            return success(xor_crypt::compiletime_version);
        }

        file_r input{ crypt.input, 0 };
        file_r pad{ crypt.pad.path, crypt.pad.pos };
        file_w output{ crypt.output };

        if ((pad.size() - crypt.pad.pos) < input.size()) {
            throw std::runtime_error("Error: The pad file is too small for the "
                                     "given pad position and input file.\n");
        }

        for (std::streamoff _ = 0; _ < input.size(); ++_) {
            u_char xor_result = static_cast<u_char>(input.get())
                                ^ static_cast<u_char>(pad.get());
            output.put(static_cast<char>(xor_result));
        }
    }
    catch (const std::runtime_error& e) {
        return failure(e.what());
    }

    return success();
}
