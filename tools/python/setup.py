from setuptools import setup

setup(name='odin-data',
      version='0.1',
      description='PERCIVAL Detector Frame Receiver',
      url='https//github.com/percival-detector/odin-data',
      author='Tim Nicholls',
      author_email='tim.nicholls@stfc.ac.uk',
      packages=[
        'emulator_client',
        'frame_processor',
        'frame_producer',
        'frame_receiver_client',
        'port_counters',
        'tools'        
      ],
      install_requires=[
        'pyzmq>=14.0.0',
        'posix_ipc>=1.0.0',
        'numpy>=1.9.1',
        'h5py>=2.5.0'
      ],
      entry_points={
        'console_scripts' : [
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