#!/bin/sh

docker run --rm -v ./spoofproxy.c:/build/spoofproxy.c -v ./Makefile:/build/Makefile -v ./out:/build/out natechoe/deb-builder
