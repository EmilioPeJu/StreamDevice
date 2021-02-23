import os.path
from iocbuilder import Device, RecordFactory, IocDataFile, ModuleBase
from iocbuilder import records, hardware, iocwriter
from iocbuilder.configure import Call_TargetOS
from iocbuilder.iocwriter import IocWriter
from iocbuilder.iocinit import quote_IOC_string, iocInit

from iocbuilder.modules.asyn import Asyn


__all__ = [
    'streamProtocol', 'ProtocolFile', 'AutoProtocol',
    'streamDeviceVersion']


assert not hasattr(hardware, 'streamDeviceVersion'), \
    'Cannot mix stream device versions!'
streamDeviceVersion = 2


class streamDevice(Device):
    LibFileList = ['pcre', 'stream']
    DbdFileList = ['stream']
    Dependencies = (Asyn,)
    AutoInstantiate = True


class streamProtocol(Device):
    Dependencies = (streamDevice,)

    def __init__(self, port, protocol):
        '''Each streamProtocol instance is constructed by binding a serial
        port to a protocol file.'''
        self.__super.__init__()

        # The new stream device requires that the stream be wrapped as an
        # asyn device.  We validate that wrapping here.
        assert getattr(port, 'IsAsyn', False), \
            'Stream Device port must be asyn port'
        self.port = port
        # Pick up the protocol name from the file name.
        self.ProtocolName = protocol.ProtocolName

        # Build the record factories for this channel
        for list, link in (
                (self.__InRecords,  'INP'),
                (self.__OutRecords, 'OUT')):
            for record in list:
                setattr(self, record, RecordFactory(
                    getattr(records, record), 'stream', link, self._address))

    # Record factory support
    def _address(self, fields, command, *protocol_args):
        if protocol_args:
            command = '%s(%s)' % (command, ','.join(map(str, protocol_args)))
        return '@%s %s %s' % (self.ProtocolName, command, self.port)

    # Lists of input and output records to generate factories for
    __InRecords = [
        'ai', 'bi', 'mbbi', 'mbbiDirect', 'longin', 'stringin', 'waveform']
    __OutRecords = [
        'ao', 'bo', 'mbbo', 'mbboDirect', 'longout', 'stringout']


class ProtocolFile(Device):
    Dependencies = (streamDevice,)
    InitialisationPhase = Device.FIRST

    # We'll need to post process the list of instances
    __CopiedFiles = set()
    __Modules = set()


    def __init__(self, protocol_file, module=None):
        self.__super.__init__()

        self.filename = protocol_file
        self.module = module
        # Pick up the protocol name from the file name.
        self.ProtocolName = os.path.basename(protocol_file)

        if module is None:
            # This file will need to be copied to the IOC data directory
            self.__CopiedFiles.add(IocDataFile(protocol_file))
        else:
            self.__Modules.add(module.MacroName())

    def InitialiseOnce(self):
        # Figure out whether we need to copy the files.  If any protocol
        # needs to be copied or if we are trying to use more than one
        # protocol directory then copying is needed.  (We could specify a
        # protocol path instead, but this isn't implemented yet.)
        print('# Configure StreamDevice paths')
        Call_TargetOS(self, 'ProtocolPath')

    def ProtocolPath_linux(self):
        protocol_dirs = ['$(%s)/data' % x for x in self.__Modules]
        if self.__CopiedFiles:
            protocol_dirs.insert(0, IocDataFile.GetDataPath())
        print('epicsEnvSet "STREAM_PROTOCOL_PATH", "%s"' % \
            ':'.join(protocol_dirs))

    def ProtocolPath_vxWorks(self):
        print('STREAM_PROTOCOL_PATH = calloc(1, %d)' % \
            (IocWriter.IOCmaxLineLength_vxWorks * (1 + len(self.__Modules))))
        sep = False
        if self.__CopiedFiles:
            print('strcat(STREAM_PROTOCOL_PATH,%s)' % \
                quote_IOC_string(IocDataFile.GetDataPath()))
            sep = bool(self.__Modules)
        for module in self.__Modules:
            if sep:
                print('strcat(STREAM_PROTOCOL_PATH,":")')
            if iocInit.substitute_boot:
                print('strcat(STREAM_PROTOCOL_PATH,"$(%s)/data")' % module)
            else:
                print('strcat(STREAM_PROTOCOL_PATH,getenv(%s))' % \
                    quote_IOC_string(module))
                print('strcat(STREAM_PROTOCOL_PATH,"/data")')
            sep = True

    def __str__(self):
        return self.ProtocolName


class AutoProtocol(ModuleBase):
    BaseClass = True

    @classmethod
    def UseModule(cls):
        # Automatically convert all protocol files into protocol instances
        # before the class is actually instantiated.
        cls.Protocols = [
            ProtocolFile(
                cls.ModuleFile(os.path.join('data', file)),
                module = cls.ModuleVersion)
            for file in cls.ProtocolFiles]
        super(AutoProtocol, cls).UseModule()
