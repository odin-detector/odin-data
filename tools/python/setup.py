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
            'frame_receiver_client = odin_data.frame_receiver.client:main',
            'meta_writer = odin_data.meta_writer.meta_writer_app:main',
         ]
      },
      zip_safe=False,
)
