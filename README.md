# Webcom C SDK

## Build prerequisites

On a debian-family platform:

```
# apt-get install cmake build-essential libjson-c-dev doxygen
```

Then install [nopoll](http://www.aspl.es/nopoll/index.html), using the provided apt repositories:

```
# echo "deb http://www.aspl.es/debian/public <DISTRIBUTION>" > /etc/apt/sources.list.d
# apt update
# apt install libnopoll0 libnopoll0-dev
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
