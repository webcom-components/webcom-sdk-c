# Webcom C SDK

![Legorange Demo Image](res/legorange-demo.gif)

The **Webcom C SDK** allows the C developers to easily interact with a Webcom
server, such as the ones operated by the
[Orange Flexible Datasync](https://datasync.orange.com/) service.

It is licensed under the LGPL V2.1. We appreciate contributions!

The Webcom C SDK has been successfully tested on various linux platforms,
including GLibc-based ones, and musl libc ones. It was also, successfully
tested on FreeBSD 11.1.  If you run it on another platform, kindly let us know
so that we can share the information. If you made some modifications to make it
work on your platform, please consider opening a pull request so it gets
integrated here.

## Documentation

The doxygen documentation for the master branch is published at this address:

https://webcom-components.github.io/webcom-sdk-c/

## General design

This SDK is meant to be used in a event-loop, and does not provide its own
event loop.

### Using an event library

An integration with libev is provided but optional. This integration takes care
of all the complicated tasks, dealing with the sockets I/O events, and the
various timers (keepalive packet timer, reconnect timer).

**Example:**

```c
#include <webcom-c/webcom.h>
#include <webcom-c/webcom-libev.h>

int main(int argc, char *argv[]) {
	wc_context_t *ctx;
	
	// get a libev event loop
	struct ev_loop *loop = EV_DEFAULT;
	
	// set a bunch of general-purpose datasync callbacks
	struct wc_eli_callbacks cb = {
			.on_connected = on_connected,
			.on_disconnected = on_disconnected,
			.on_error = on_error,
	};
    // set the connection options
	struct wc_context_options options = {
			.host = "io.datasync.orange.com",
			.port = 443,
			.app_name = "my_namespace",
	};

	// establish the connection to the webcom server, and let it integrate in
	// our libev event loop
	ctx = wc_context_create_with_libev(
			&options,
			loop,
			&cb);

    // we are going to use the datasync service
	wc_datasync_init(ctx);
	
	// ask the SDK to establish the connection to the server
	wc_datasync_connect(ctx);

	// enter the event loop
	ev_run(loop, 0);
	
	// destroy the context when the loop ends
	wc_context_destroy(ctx);

	return 0;
}

// called when the connection to the server is established
static void on_connected(wc_context_t *ctx) {
	wc_datasync_on_child_added(ctx, "/foo/bar", on_foo_bar_added);
}

// called whenever a new child node is appended to /foo/bar
int on_foo_bar_added(wc_context_t *ctx, on_handle_t handle, char *data, char *cur, char *prev) {
	printf("a new child named [%s] with value [%s] was added on path /foo/bar\n", cur, data);
	return 1;
}

(...)
```

### Without any provided event library integration

If you don't rely on the libevent library integration, there is more to de
don on your side:

1. establish a connection to the Webcom server using `wc_context_new()`, pass
   a function pointer that will receive the "low level" events to handle,
2. wait for `WC_EVENT_*_FD` events to know which file descriptors to add,
   remove, or modify in your select/poll/epoll set, and wait for the
   `WC_SET_TIMER` events
3. poll that file descriptor in your event loop for write events,
4. call the internal event dispatching methods `wc_dispatch_fd_event()` and
   `wc_dispatch_timer_event()` when an event occurs.

**Note:** The Webcom server **will close any inactive connection** after a 1
minute timeout. The event loop is responsible for calling `wc_cnx_keepalive()`
periodically (e.g. every 50 seconds) to keep the connection alive.

### Integrate with CMake

The SDK can be conveninently used in a CMake project using:

```
find_package(webcom-c)
```

## Get the SDK

There are serveral ways of getting this SDK. The easiest one is to download a
precompiled package for your distribution. If we don't provide a package for
your distribution, you can build it from the sources, directly on your host, or
using a dedicated docker image.

### Install the pre-compiled package

Choose one package corresponding to your hardware/distribution from the
Github **releases** tab.

### Docker build images

Some Docker images have been crafted in [build-images/](build-images/), to build
the Webcom C SDK for several distributions. If you want to build your own
package of your own version of the SDK, using these images make it rather
straighforward.

### Build manually from Git

#### Build prerequisites

(**Note:** you can also take a look the Dockerfiles in
[build-images/](build-images/) to get an idea of the build requirements)

##### On a debian-family platform (Debian >= 9, Ubuntu >= 16.04)

```
# apt-get install cmake gcc cmake make pkg-config libjson-c-dev libwebsockets-dev libev-dev libncurses5-dev doxygen libcurl4-openssl-dev libreadline-dev libsystemd-dev
```

##### On a RedHat-family platform (CentOS or RHEL >= 7, Fedora Core >= 24)

for CentOS 7 and RHEL 7

```
# yum install epel-release
# yum install cmake3

```

for fedora core >= 24

```
# yum install cmake
```

then

```
# yum install gcc make pkgconfig json-c-devel json-c-devel libwebsockets-devel openssl-devel systemd-devel libev-devel libcurl-devel readline-devel doxygen
```

#### Build process

```
$ git clone [this repo URL]
(...)
$ cd webcom-sdk-c/build
$ cmake ..
(use `cmake3` instead of cmake) under CentOS 7 and RHEL 7)
(...)
$ make all
(...)
```
It will produce **libwebcom-c.so** in the **lib/** subdirectory, plus the
examples, documentation, and the test programs.

#### Install

```
[still in the build/ folder]
$ sudo make install
```

## Man pages / HTML documentation

The documentation is produced by doxygen (the public .h files contain the
documentation). The binary packages and manual installation do install the
man pages, that document the SDK API. You can also generate the documentation
in HTML format (requires graphviz):

```
[still in the build/ folder]
$ cmake .. -DBUILD_HTML_DOCUMENTATION=ON
$ make doc
(builds the HTML doc in doc/html/)
```

## Demos

This SDK comes with two examples: **legorange** and **wcchat** in the
[examples/](examples/) directory. They are the C counterparts of
[Legorange](https://io.datasync.orange.com/samples/legorange/) and
[WebCom Simple Chat](https://io.datasync.orange.com/samples/chat/). These demos
are automatically built and installed along with the SDK, and their source code
are meant to be a source of information and inspiration on how to start hacking
with the SDK.

