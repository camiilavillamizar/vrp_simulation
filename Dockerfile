FROM ubuntu:22.04

RUN apt-get update && apt-get install -y libjson-c-dev

RUN apt-get update && \
    apt-get install -y build-essential gcc g++ libopenmpi-dev openmpi-bin openmpi-common make nano

WORKDIR /workspace

ENV LANG C.UTF-8