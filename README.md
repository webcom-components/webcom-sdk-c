# Webcom C SDK

## Build prerequisites

On a debian-family platform:

```
# apt-get install cmake libjson-c-dev libwebsockets-dev doxygen
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

## Documentation

The documentation is produced by doxygen:

```
[still in the build/ folder]
$ make doc
(...)
```

## Test

```
[still in the build/ folder]
$ make all test
(...)
```
