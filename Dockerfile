FROM rockylinux:9  AS common

ENV PATH=/odin/bin:/odin/scripts:/venv/bin:$PATH

# Get fundamental packages
RUN dnf update -y && \
    dnf install  -y \
    tar ca-certificates  && \
    dnf -y clean all

# Get zellij
RUN curl -L https://github.com/zellij-org/zellij/releases/download/v0.40.1/zellij-x86_64-unknown-linux-musl.tar.gz -o zellij.tar.gz && \
    tar -xvf zellij.tar.gz -C /usr/bin && \
    rm zellij.tar.gz
RUN mkdir -p ~/.config/zellij && \
    zellij setup --dump-config > ~/.config/zellij/config.kdl

# Developer stage for devcontainer #############################################
FROM common AS developer

# System dependencies

RUN dnf install -y epel-release && \
    dnf config-manager --set-enabled crb
RUN dnf group install -y "Development Tools"
RUN dnf update -y && dnf install -y  \
    # General build
    cmake git \
    # odin-data C++ dependencies
    blosc-devel boost-devel hdf5-devel log4cxx-devel libpcap-devel czmq-devel \
    # python
    python3.11-devel python3.11-pip \
    # clang tools
    clang20-tools-extra \
    # debugging
    gdb valgrind && \
    # tidy up
    dnf -y clean all


# Python dependencies
RUN python3.11 -m ensurepip && \
    python3.11 -m venv /venv && \
    /venv/bin/python -m pip install --upgrade pip && \
    /venv/bin/python -m pip install git+https://github.com/odin-detector/odin-control

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
RUN python3.11 -m pip install /odin/odin-data/python[meta_writer]

# Runtime stage ################################################################
FROM common AS runtime

# Runtime system dependencies
RUN dnf install -y epel-release && \
    dnf config-manager --set-enabled crb
RUN dnf update -y && dnf install -y \
    # C++ dependencies
    blosc-devel boost-devel hdf5-devel log4cxx-devel libpcap-devel czmq-devel \
    # Python dependencies
    python3.11-devel && \
    # Tidy up
    dnf -y clean all

COPY --from=build /odin /odin
COPY --from=build /venv /venv

RUN rm -rf /odin/odin-data /odin/odin-control

WORKDIR /odin
