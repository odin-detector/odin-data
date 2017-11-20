"""Setup script for odin_data python package."""

import sys
from setuptools import setup, find_packages
import versioneer

with open('requirements.txt') as f:
    required = f.read().splitlines()

setup(name='odin_data',
      version=versioneer.get_version(),
      cmdclass=versioneer.get_cmdclass(),
      description='ODIN Data Framework',
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
            'frame_receiver_client = odin_data.frame_receiver.client:main',
            'frame_processor_client = odin_data.frame_processor.client:main',
         ]
      },
      zip_safe=False,
)
