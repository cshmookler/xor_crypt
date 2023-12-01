# **XOR Crypt**

A command-line utility for encrypting files with a one-time pad. Each bit in the input file is XOR'd with the cooresponding bit in the one-time pad. 

## **Proper Usage**

**Show proper usage for the installed version of xor_crypt.**

```bash
xorc --help
```

**Show installed xor_crypt version.**

```bash
xorc --version
```

**Encrypt/decrypt a file**

```bash
xorc <input_file> <output_file> [--pad=<path_to_pad>] [--pos=<position_in_pad>]
```

> **NOTE**: Options (starting with the "-\-" prefix) are optional and can be in any order.

> **WARNING**: Paths to files must be absolute. Relative paths will NOT be normalized.

**Example:**\
Encrypt "my_secret.txt" with "my_pad.key" at position 0 and write output to "my_encrypted_secret.crypt".

```bash
xorc my_secret.txt my_encrypted_secret.crypt --pad=my_pad.key --pos=0
```

## **Build and install this project with Conan (for Unix-like systems)**

**1.** Install a C++ compiler (Example: clang), Git, and Python >=3.7 (Example: apt).

```bash
sudo apt install -y clang git python3
```

**2.** (Optional) Create a Python >=3.7 virtual environment and activate it. Install the Python Virtual Environment if you haven't already.

```bash
python3 -m venv .venv
source .venv/bin/activate
```

**3.** Install Conan.

```bash
pip3 install "conan>=2.0.0"
```

**4.** Create the default Conan profile.

```bash
conan profile detect
```

**5.** Build and install with Conan.

```bash
conan create .
```
