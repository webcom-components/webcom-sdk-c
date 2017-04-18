# Webcom C SDK

## Prerequisites

On a debian-family platform:

```
# apt-get install cmake libjson-c-dev libwebsockets-dev
```

## Build

```
$ cd build
$ cmake ..
(...)
$ make all
(...)
```

It will produce **libwebcom-c.so** in the current directory

## Test

```
[still in the build/ folder]
$ make all test
(...)
```
