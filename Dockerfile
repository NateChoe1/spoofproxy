FROM debian:stable-slim

RUN apt-get update && apt-get upgrade -y && apt-get install -y build-essential

# hack to create two dirs
RUN mkdir -p /build/out

WORKDIR /build
ENTRYPOINT make clean && make
