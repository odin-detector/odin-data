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
# # Final image
# FROM fedora:37
# COPY --from=build /odin /odin
# Ask Giles about build and run images

WORKDIR /odin
