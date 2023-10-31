# **XOR Crypt**

A commmand-line utility for encrypting files using a one-time pad. Each bit in the input file is XOR'd with the cooresponding bit in the one-time pad.

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
