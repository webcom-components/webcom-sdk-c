FROM debian:9

RUN apt-get update

RUN apt-get -y install git

# build tools
RUN apt-get -y install gcc cmake make pkg-config

# SDK build dependencies
RUN apt-get -y install libjson-c-dev libwebsockets-dev libsystemd-dev libcurl4-openssl-dev

# examples build dependencies
RUN apt-get -y install libev-dev libreadline-dev

# event loop integrations
RUN apt -y install libev-dev libuv1-dev libevent-dev

# documentation build dependencies
RUN apt-get -y install doxygen

RUN apt-get -y install dpkg-dev

RUN apt-get -y clean

VOLUME /tmp/webcom-sdk-c/
