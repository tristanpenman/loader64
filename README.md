# Loader 64

A simple command line tool for uploading data to the EverDrive-64.

This is based on code by saturnu `<tt@anpa.nl>`, which was originally shared at:
http://krikzz.com/forum/index.php?topic=1407.msg14076

It was later updated by James Friend (https://github.com/jsdf/loader64) to work on macOS.

This version includes various changes to make it work with EverDrive OS V3.05, and to simplify the command line interface.

## macOS

### Build from source

Install required libraries from Homebrew:

```bash
brew install libftdi libusb
```

Build the `loader64` executable:

```bash
make loader64
```

Load a ROM:

```bash
./loader64 -v -f myRom.n64
```

## Linux

Install `libftdi` from your package manager of choice (including headers). For example (on Ubuntu):

```bash
sudo apt-get install libftdi1 libftdi1-dev
```

Build the `loader64` executable:

```bash
make loader64
```

Load a ROM:

```bash
./loader64 -v -f myRom.n64
```