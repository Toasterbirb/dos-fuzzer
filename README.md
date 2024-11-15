# dos-fuzzer

dos-fuzzer is a tool for finding flaws in file parsing that results in crashes and/or DoS in the form of the file parser freezing. This is done by randomly changing portions of the file and seeing how the program being tested reacts to it. Flaws are detected from the return value and the execution time of the program.

Since part of the testing involves parsing possibly corrupted files, the outcomes might be less than desired. One such bad outcome might be resource exhaustion DoS for example and cause the host running the fuzzing to crash. Thus it is recommended to run tests in a virtual machine or in some kind of a contained environment.

## Usage
Command usage
```
dos_fuzzer [command] [path_to_binary] [section_address] [section_size]
```
The `[command]` is the full command that should be run with the patched file. In the command string the file being fuzzed should be replaced with `%c`. For example `r2 -q %c` would run radare2 on the patched binary. To make the output cleaner, it is recommended to redirect all output of the command to `/dev/null`.

`[path_to_binary]` should be fairly explanatory (it is the file we want to fuzz with)

`[section_address]` and `[section_size]` refer to the portion of the binary we are going to fuzz. Both values should be provided in hexadecimal form. The section\_address value is the beginning of the portion in the binary to be fuzzed and section\_size sets the range. If you want to fuzz the entire binary, the address could probably be 0x0 and the size the size of the binary.

## Building
Build the project with g++ by running `make`. To speed up the build, you can try using the -j flag.
```sh
make -j$(nproc)
```

## Installation
To install dos-fuzzer to /usr/local/bin, run the following
```sh
make install
```
You can customize the installation prefix with the PREFIX variable like so
```sh
make PREFIX=/usr install
```

## Uninstall
```sh
make uninstall
```
