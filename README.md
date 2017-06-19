# Webcom C SDK

## Build prerequisites

On a debian-family platform:

```
# apt-get install cmake build-essential libjson-c-dev libev-dev libev4 doxygen
```

Then install [nopoll](http://www.aspl.es/nopoll/index.html), using the [provided apt repositories](http://www.aspl.es/nopoll/downloads.html):

```
# echo "deb http://www.aspl.es/debian/public <DISTRIBUTION>" > /etc/apt/sources.list.d/nopoll.list
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

## Install

```
[still in the build/ folder]
$ sudo make install
```

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
