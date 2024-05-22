FROM fedora:37 AS build

RUN dnf update -y && dnf install -y \
    # General build
    gcc-c++ make cmake git \
    # odin-data C++ dependencies
    blosc-devel boost-devel hdf5-devel log4cxx-devel libpcap-devel zeromq-devel \
    # python
    python3-devel python3-pip && \
    # tidy up
    dnf -y clean all

# Root of odin-data
COPY . /tmp/odin-data

# C++
WORKDIR /tmp/odin-data
ENV PATH=/odin/bin:$PATH
RUN mkdir /odin && \
    mkdir -p build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/odin ../cpp && \
    make -j8 VERBOSE=1 && \
    make install

# Python
WORKDIR /tmp/odin-data/python
ENV PATH=/odin/venv/bin:$PATH
RUN python3 -m venv /odin/venv && \
    python -m pip install --upgrade pip && \
    python -m pip install git+https://github.com/odin-detector/odin-control@1.3.0 && \
    python -m pip install .[meta_writer]

RUN rm -rf /tmp/odin-data

# Runtime Image
FROM fedora:37 as runtime

# Enable extra package repositories
RUN dnf install -y 'dnf-command(copr)' && \
    dnf copr enable -y varlad/zellij

RUN dnf update -y && \dnf install -y \
    # odin-data C++ runtime dependencies
    blosc boost hdf5 log4cxx libpcap zeromq \
    # Useful tools
    zellij && \
    # tidy up
    dnf -y clean all

COPY --from=build /odin /odin

ENV PATH=/odin/bin:/odin/venv/bin:$PATH

WORKDIR /odin
