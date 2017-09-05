"""Setup script for odin_data python package."""

import sys
from setuptools import setup, find_packages
import versioneer

with open('requirements.txt') as f:
    required = f.read().splitlines()

if sys.version_info[0] == 2:
    required.append('futures>=3.0.0')

setup(name='odin_data',
      version=versioneer.get_version(),
      cmdclass=versioneer.get_cmdclass(),
      description='PERCIVAL Detector Frame Receiver',
      url='https//github.com/odin-detector/odin-data',
      author='Tim Nicholls',
      author_email='tim.nicholls@stfc.ac.uk',
      packages=find_packages(),
      install_requires=required,
      entry_points={
        'console_scripts': [
            'emulator_client = emulator_client.emulator_client:main',
            'frame_processor = frame_processor.frame_processor:main',
            'frame_producer = frame_producer.frame_producer:main',
            'frame_receiver_client = frame_receiver_client.frame_receiver_client:main',
            'port_counters = port_counters.port_counters:main',
            'decode_raw_frames_hdf5 = tools.decode_raw_frames_hdf5:main',
         ]
      },
      zip_safe=False,
)
