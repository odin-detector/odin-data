# odin-data

[![cpp-test](https://github.com/odin-detector/odin-data/actions/workflows/_cpp_test.yml/badge.svg)](https://github.com/odin-detector/odin-data/actions/workflows/_cpp_test.yml)
[![docs-build](https://github.com/odin-detector/odin-data/actions/workflows/_docs.yml/badge.svg)](https://github.com/odin-detector/odin-data/actions/workflows/_docs.yml)
[![codecov](https://codecov.io/gh/odin-detector/odin-data/branch/master/graph/badge.svg?token=Urucx8wsTU)](https://codecov.io/gh/odin-detector/odin-data)
[![Apache License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

odin-data is a modular, scalable, high-throughput data acquisition framework consisting
of two communicating applications; the FrameReceiver and FrameProcessor. It acts as a
data capture and processing pipeline for detector data streams transferred over a
network connection. The FrameReceiver constructs data frames in shared memory buffers
and the FrameProcessor performs processing on the buffer and writes data to disk.

Notes to Contributors:

1. To allow sharing of git hooks they have been stored in the directory '.githooks'.
For them to be used you MUST run the following command (only needs to be run once)

Either:

    Build (or rebuild) the dev-container in VScode, and the hooks are a automatically setup.

Or:
    Run
    `git config core.hooksPath .githooks`
    If not developing using the dev container.

2. In order to allow formatting changes (and possibly other things) to be made to the codebase,
without too much disruption to the efficacy of the git-blame the following procedure can
be used.

There is a file called .git-blame-ignore-revs in the root of the repository that can be
updated to record each commit that should be ignored for git-blame purposes.

To actually set this up so that it is used by git-blame you need to the run the following
command (it only needs to be run once as the information is then stored in .git/config
for this repository)

Either:

   Build (or rebuild) the dev-container in VScode, and the tools are a automatically setup.

Or:

   git config blame.ignoreRevsFile .git-blame-ignore-revs
