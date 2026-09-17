# Base image
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    wget \
    dos2unix \
    xz-utils \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Download OpenWatcom snapshot
RUN wget -O /tmp/ow-snapshot.tar.xz \
    https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/ow-snapshot.tar.xz

# Extract directly into /opt/openwatcom/ow
RUN mkdir -p /opt/openwatcom/ow && \
    tar -xJf /tmp/ow-snapshot.tar.xz -C /opt/openwatcom/ow --strip-components=1 && \
    rm /tmp/ow-snapshot.tar.xz

# Set environment variables
ENV WATCOM=/opt/openwatcom/ow
ENV PATH=$WATCOM/binl:$PATH
ENV INCLUDE=$WATCOM/h
ENV LIB=$WATCOM/lib386
ENV EDPATH=$WATCOM/eddat

# Default workdir inside container
WORKDIR /src

# Entry point
ENTRYPOINT ["/bin/bash", "-c"]
