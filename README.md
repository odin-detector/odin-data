# odin-data

[![C++ CI](https://github.com/odin-detector/odin-data/actions/workflows/_cpp_test.yml/badge.svg?branch=master)](https://github.com/odin-detector/odin-data/actions/workflows/_cpp_test.yml)
[![Docs CI](https://github.com/odin-detector/odin-data/actions/workflows/_docs.yml/badge.svg?branch=master)](https://github.com/odin-detector/odin-data/actions/workflows/_docs.yml)
[![codecov](https://codecov.io/gh/odin-detector/odin-data/branch/master/graph/badge.svg?token=Urucx8wsTU)](https://codecov.io/gh/odin-detector/odin-data)
[![Apache License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

odin-data is a modular, scalable, high-throughput data acquisition framework consisting
of two communicating applications; the FrameReceiver and FrameProcessor. It acts as a
data capture and processing pipeline for detector data streams transferred over a
network connection. The FrameReceiver constructs data frames in shared memory buffers
and the FrameProcessor performs processing on the buffer and writes data to disk.
