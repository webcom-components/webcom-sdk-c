# Webcom boilerplate project using CMake

This "boilerplate" project gives an example and a starting point for a simple C
project using the Webcom C SDK. It assumes the event loop is handled by libev,
and the JSON data manipulation will be done through json-c. Feel free to copy
this skeleton and adapt it to your needs.

## Prerequisites

- the Webcom C SDK
- [libev](http://software.schmorp.de/pkg/libev.html)
- [libjson-c](https://github.com/json-c/json-c)
- [CMake](https://cmake.org/)

## Build

```
cd build
cmake ..
make
```
