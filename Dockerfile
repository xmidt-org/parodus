# if latest docker version caches the layers and doesn't force a build, try old docker:
# DOCKER_BUILDKIT=0  docker build --no-cache -t parodus . 

# build stage
FROM ubuntu:20.04 AS packager

RUN apt-get \
    -y --allow-downgrades --allow-remove-essential --allow-change-held-packages \
    update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    git \
    cmake \
    autoconf \
    make \
    musl-dev \
    gcc \
    g++ \
    openssl \
    libssl-dev \
    libcurl4-openssl-dev \
    libcunit1 \
    libcunit1-doc \
    libcunit1-dev \ 
    automake  \
    libtool \
    uuid-dev \
    pkg-config \
    build-essential \
    clang \
    gdb \
    wget 
    #apk add --no-cache cmake autoconf make musl-dev gcc g++ openssl openssl-dev git cunit cunit-dev automake libtool util-linux-dev && \

COPY . .

WORKDIR /

RUN rm -rf build 
RUN mkdir build


RUN cd build && \
    cmake ..  && make 

#build image
FROM ubuntu:20.04

WORKDIR /
COPY --from=packager /build/src/parodus /usr/bin/
COPY --from=packager /build/_install/lib/libcimplog.so.1.0.0 /usr/lib/
COPY --from=packager /build/_install/lib/libcjson.so.1.7.8 /usr/lib/
COPY --from=packager /build/_install/lib/libcjwt.so /usr/lib/
COPY --from=packager /build/_install/lib/libmsgpackc.so.2.0.0 /usr/lib/
COPY --from=packager /build/_install/lib/libnopoll.so.0.0.0 /usr/lib/
COPY --from=packager /build/_install/lib/libtrower-base64.so.1.0.0 /usr/lib/
COPY --from=packager /build/_install/lib/libwrp-c.so /usr/lib/
COPY --from=packager /build/_install/lib/libnanomsg.so.5.1.0 /usr/lib/
COPY --from=packager /build/_install/lib/libcurl.so /usr/lib/
RUN chmod uga+x /usr/bin/parodus && \
    apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y openssl util-linux && \
    ln -s /usr/lib/libcurl.so /usr/lib/libcurl.so.1 && \
    ln -s /usr/lib/libmsgpackc.so.2.0.0 /usr/lib/libmsgpackc.so && \
    #ln -s /usr/lib/libmsgpackc.so.2.0.0 /usr/lib/libmsgpackc.so.2 && \
    ln -s /usr/lib/libtrower-base64.so.1.0.0 /usr/lib/libtrower-base64.so && \
    ln -s /usr/lib/libnopoll.so.0.0.0 /usr/lib/libnopoll.so && \
    #ln -s /usr/lib/libnopoll.so.0.0.0 /usr/lib/libnopoll.so.0 && \
    ln -s /usr/lib/libcimplog.so.1.0.0 /usr/lib/libcimplog.so && \
    #ln -s /usr/lib/libnanomsg.so.5.1.0 /usr/lib/libnanomsg.so.5 && \
    ln -s /usr/lib/libnanomsg.so.5 /usr/lib/libnanomsg.so && \
    #ln -s /usr/lib/libcjson.so.1.7.8 /usr/lib/libcjson.so.1 && \
    ln -s /usr/lib/libcjson.so.1 /usr/lib/libcjson.so
ENTRYPOINT [ "/usr/bin/parodus" ]
