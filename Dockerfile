FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential gcc g++ make nano \
    libjson-c-dev libopenmpi-dev openmpi-bin openmpi-common \
    python3
    
WORKDIR /workspace

ENV LANG C.UTF-8