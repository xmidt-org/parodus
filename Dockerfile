# build stage
FROM alpine:3.12

RUN \
    apk add --no-cache linux-headers cmake autoconf make musl-dev gcc g++ openssl openssl-dev git cunit cunit-dev automake libtool util-linux-dev && \
    mkdir -p build


COPY src src
COPY patches patches
COPY tests tests
COPY CMakeLists.txt .
COPY run.sh .


RUN cd build && \
    cmake .. && make 

CMD ["build/src/parodus"]