# dos-fuzzer

dos-fuzzer is a tool for finding flaws in file parsing that results in crashes and/or DoS in the form of the file parser freezing. This is done by randomly changing portions of the file and seeing how the program being tested reacts to it. Flaws are detected from the return value and the execution time of the program.

Since part of the testing involves parsing possibly corrupted files, the outcomes might be less than desired. One such bad outcome might be resource exhaustion DoS for example and cause the host running the fuzzing to crash. Thus it is recommended to run tests in a virtual machine or in some kind of a contained environment.

## Usage
See the output of `dos-fuzzer --help`

### Output format explanation
Here's one possible line of output:
```
0x3756 | ret 5228ms | 3d b9 0b 53 97 a7 8a 8e 48 29 d4 18 e6 5f 98 0d 0d 82 d8 58 02 cd f6
```
The first column is the location where the string of bytes would be in the patched binary.

The middle column shows the return result of the execution. If the return value was non-zero, it'll contain the word `ret`. Following it is the execution time measured in milliseconds.

The last column shows an array of bytes that were patched into the binary started from the address shown in the first column.

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
