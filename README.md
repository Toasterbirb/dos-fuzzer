# dos-fuzzer

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
