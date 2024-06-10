# Shared setup stage ###########################################################
FROM ubuntu:24.04 AS common

ENV PATH=/odin/bin:/venv/bin:$PATH

# Get fundamental packages
RUN apt-get update -y && \
    apt-get install --no-install-recommends -y curl \
    tar ca-certificates software-properties-common && \
    apt-get -y clean all

# Get zellij
RUN curl -L https://github.com/zellij-org/zellij/releases/download/v0.40.1/zellij-x86_64-unknown-linux-musl.tar.gz -o zellij.tar.gz && \
    tar -xvf zellij.tar.gz -C /usr/bin && \
    rm zellij.tar.gz
RUN mkdir -p ~/.config/zellij && \
    zellij setup --dump-config > ~/.config/zellij/config.kdl

# Developer stage for devcontainer #############################################
FROM common AS developer

# System dependencies
RUN add-apt-repository -y ppa:deadsnakes/ppa && \
    apt-get update -y && apt-get install -y --no-install-recommends \
    # General build
    build-essential cmake git \
    # odin-data C++ dependencies
    libblosc-dev libboost-all-dev libhdf5-dev liblog4cxx-dev libpcap-dev libczmq-dev \
    # python
    python3.11-dev python3.11-venv && \
    # tidy up
    apt-get -y clean all

# Python dependencies
RUN python3.11 -m ensurepip && \
    python3.11 -m venv /venv && \
    python -m pip install --upgrade pip && \
    python -m pip install git+https://github.com/odin-detector/odin-control

# Install hdf5filters from source
RUN git clone https://github.com/DiamondLightSource/hdf5filters.git && cd hdf5filters && \
    mkdir -p cmake-build && cd cmake-build && \
    cmake -DCMAKE_INSTALL_PREFIX=/odin -DCMAKE_BUILD_TYPE=Release -DUSE_AVX2=ON .. && \
    make install && \
    rm -rf hdf5filters

# Build stage - throwaway stage for runtime assets #############################
FROM developer AS build

# Copy in project
WORKDIR /odin/odin-data
COPY . .

# C++
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/odin ../cpp && \
    make -j8 VERBOSE=1 && \
    make install

# Python
RUN python -m pip install /odin/odin-data/python[meta_writer]

# Runtime stage ################################################################
FROM common AS runtime

# Runtime system dependencies
RUN add-apt-repository -y ppa:deadsnakes/ppa && \
    apt-get update -y && apt-get install -y --no-install-recommends \
    # C++ dependencies
    libblosc-dev libboost-all-dev libhdf5-dev liblog4cxx-dev libpcap-dev libczmq-dev \
    # Python dependencies
    python3.11 && \
    # Tidy up
    apt-get -y clean all

COPY --from=build /odin /odin
COPY --from=build /venv /venv

RUN rm -rf /odin/odin-data /odin/odin-control

WORKDIR /odin
