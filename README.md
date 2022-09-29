# odin-data

[![Code CI](https://github.com/odin-detector/odin-data/actions/workflows/code.yml/badge.svg?branch=docs-etc)](https://github.com/odin-detector/odin-data/actions/workflows/code.yml)
[![Docs CI](https://github.com/odin-detector/odin-data/actions/workflows/docs.yml/badge.svg?branch=docs-etc)](https://github.com/odin-detector/odin-data/actions/workflows/docs.yml)
[![Code Climate](https://codeclimate.com/github/odin-detector/odin-data/badges/gpa.svg)](https://codeclimate.com/github/odin-detector/odin-data)
[![Test Coverage](https://codeclimate.com/github/odin-detector/odin-data/badges/coverage.svg)](https://codeclimate.com/github/odin-detector/odin-data/coverage)
[![Apache License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

odin-data is a modular, scalable, high-throughput data acquisition framework consisting
of two communicating applications; the FrameReceiver and FrameProcessor. It acts as a
data capture and processing pipeline for detector data streams transferred over a
network connection. The FrameReceiver constructs data frames in shared memory buffers
and the FrameProcessor performs processing on the buffer and writes data to disk.
