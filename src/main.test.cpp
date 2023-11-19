// Standard includes
#include <cstdio>
#include <climits>
#include <string>
#include <string_view>
#include <fstream>

// External includes
#include <gtest/gtest.h>

using openmode = std::ios_base::openmode;

class File {
    std::string_view name_;

  public:
    explicit File(const std::string_view& name) : name_(name) {
        std::ofstream stream{ this->name_.data(),
                              openmode::_S_out | openmode::_S_bin };
        if (! stream.is_open()) {
            throw std::runtime_error(
                "Failed to open file\"" + std::string(this->name_)
                + "\" for writing.");
        }
        for (u_char chr = 0; chr < static_cast<u_char>(UCHAR_MAX); ++chr) {
            stream.put(static_cast<char>(chr));
        }
        stream.put(static_cast<char>(UCHAR_MAX));
    }
    File(const File&) = delete;
    File(File&&) noexcept = default;
    File& operator=(const File&) = delete;
    File& operator=(File&&) noexcept = default;
    ~File() { (void)std::remove(this->name_.data()); }

    std::string name() { return std::string(this->name_); }
};

void test_cmd(const std::string_view& cmd, const std::string_view& expected) {
    FILE* output = popen(cmd.data(), "r");
    if (output == nullptr) {
        throw std::runtime_error("Failed to start process");
    }
    std::string str;
    str.reserve(expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        int chr = fgetc(output);
        if (chr == EOF) {
            break;
        }
        str.push_back(static_cast<char>(chr));
    }
    pclose(output);
    if (str.rfind(expected.data()) != 0) {
        throw std::runtime_error(
            "actual: '" + str + "', expected: '" + std::string(expected.data())
            + "'");
    }
}

TEST(cmd, echo) {
    test_cmd("echo this should succeed", "this should succeed");
}

TEST(xorc, NoArgumentProvided) {
    test_cmd("./xorc", "Error: No argument provided.");
}

TEST(xorc, TooManyPositionalArguments) {
    test_cmd("./xorc arg1 arg2 arg3", "Error: Too many positional arguments.");
}

TEST(xorc, NegativePadPosition) {
    test_cmd(
        "./xorc arg1 arg2 --pos=-1", "Error: Pad position cannot be negative.");
}

TEST(xorc, InvalidOption) {
    test_cmd("./xorc arg1 arg2 --opt", "Error: Invalid option.");
}

TEST(xorc, EncryptTestFile) {
    File input("test_input");
    File output("pad.key");
    test_cmd("./xorc test_input test_output", "");
    test_cmd(
        "diff test_input test_output",
        "Binary files test_input and test_output differ");
    ASSERT_EQ(0, std::remove("test_output"));
}

TEST(xorc, EncryptAndDecryptTestFile) {
    File input("test_input");
    File output("pad.key");
    test_cmd("./xorc test_input test_output", "");
    test_cmd(
        "diff test_input test_output",
        "Binary files test_input and test_output differ");
    test_cmd("./xorc test_output test_output_decrypted", "");
    test_cmd("diff test_input test_output_decrypted", "");
    ASSERT_EQ(0, std::remove("test_output"));
    ASSERT_EQ(0, std::remove("test_output_decrypted"));
}
