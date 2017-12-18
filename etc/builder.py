from iocbuilder import Device
from iocbuilder.arginfo import makeArgInfo, Simple

__all__ = ['OdinData']


class OdinData(Device):

    """Store configuration for OdinData."""

    # Device attributes
    AutoInstantiate = True

    def __init__(self, MACRO, IP, READY, RELEASE, META):
        self.__super.__init__()
        # Update attributes with parameters
        self.__dict__.update(locals())

    ArgInfo = makeArgInfo(__init__,
                          MACRO=Simple('Dependency MACRO as in configure/RELEASE', str),
                          IP=Simple('IP address of server hosting preocesses', str),
                          READY=Simple('Port for Ready Channel', int),
                          RELEASE=Simple('Port for Release Channel', int),
                          META=Simple('Port for Meta Channel', int))
