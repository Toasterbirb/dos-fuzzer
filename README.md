# dos-fuzzer

dos-fuzzer is a tool for finding flaws in file parsing that results in crashes and/or DoS in the form of the file parser freezing. This is done by randomly changing portions of the file and seeing how the program being tested reacts to it. Flaws are detected from the return value and the execution time of the program.

Since part of the testing involves parsing possibly corrupted files, the outcomes might be less than desired. One such bad outcome might be resource exhaustion DoS for example and cause the host running the fuzzing to crash. Thus it is recommended to run tests in a virtual machine or in some kind of a contained environment.

## Usage
See the output of `dos-fuzzer --help`

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
