Using the python tools
===============

Several python scripts in this area can be used to emulate parts of the Percival
data acquisition system. This is a summary of the tools and instructions on how to
operate them.

Installing a Virtual Python Environment
-------------------------------------------

Ensure you have virtualenv installed in your environment first. Then create and 
activate a new virtualenv - and finally install the required dependencies  with the
following commands:

    # Move from root into this dir
    cd tools/python
    
    # First create the virtual environment.
    # This need to be done only once for your working copy
    virtualenv -p /path/to/favourite/python venv
    
    # Activate the venv - this need to be done for each shell you work in
    source venv/bin/activate
    
    # Install the required dependencies.
    # This step is only required once for each virtualenv
    pip install -r requirements.txt
    

Available tools
----------------

The following describe the available tools, and their configurations.

*frame_producer.py*

*frame_processor.py*

*emulator_client.py*


