# shared setup stage ###########################################################
FROM ubuntu:24.04 AS common

ENV PATH=/odin/bin:/venv/bin:$PATH

# get fundamental packages
RUN apt-get update -y && \
    apt-get install --no-install-recommends -y curl \
    tar ca-certificates software-properties-common && \
    apt-get -y clean all
# get zellij
RUN curl -L https://github.com/zellij-org/zellij/releases/download/v0.40.1/zellij-x86_64-unknown-linux-musl.tar.gz -o zellij.tar.gz && \
    tar -xvf zellij.tar.gz -C /usr/bin && \
    rm zellij.tar.gz
# setup zellij
RUN mkdir -p ~/.config/zellij && \
    zellij setup --dump-config > ~/.config/zellij/config.kdl

# developer stage for devcontainer #############################################
FROM common AS developer

# system depenedencies
RUN add-apt-repository -y ppa:deadsnakes/ppa && \
    apt-get update -y && apt-get install -y --no-install-recommends \
    # General build
    build-essential cmake git \
    # odin-data C++ dependencies
    libblosc-dev libboost-all-dev libhdf5-dev liblog4cxx-dev \
    libpcap-dev libczmq-dev \
    # python
    python3.11-dev python3.11-venv && \
    # tidy up
    apt-get -y clean all

# python dependencies
RUN python3.11 -m ensurepip && \
    python3.11 -m venv /venv && \
    python -m pip install --upgrade pip && \
    python -m pip install git+https://github.com/odin-detector/odin-control@1.3.0

# build stage - throwaway stage for runtime assets #############################
FROM developer AS build

# fetch the source
WORKDIR /tmp/odin-data
COPY . .

# C++
RUN mkdir /odin && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/odin ../cpp && \
    make -j8 VERBOSE=1 && \
    make install

# Python
RUN python -m pip install /tmp/odin-data/python[meta_writer]

# Runtime stage ################################################################
FROM common as runtime

# runtime system dependencies
RUN apt-get update -y && apt-get install -y --no-install-recommends \
    # odin-data C++ dependencies
    libblosc1 libboost-dev libhdf5-103 liblog4cxx12 libpcap0.8 libczmq4 && \
    # tidy up
    apt-get -y clean all

COPY --from=build /odin /odin
COPY --from=build /venv /venv

WORKDIR /odin
