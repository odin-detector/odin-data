#!/bin/env python

import os
import sys
import subprocess

subprocess.call(["python", "-m", "frame_producer", "--help"])
subprocess.call(["frameReceiver", "--help"])
subprocess.call(["python", "-m", "frame_processor", "--help"])