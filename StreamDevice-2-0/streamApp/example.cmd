#!./streamApp

epicsEnvSet "STREAM_PROTOCOL_PATH", ".:protocols:../protocols/"
dbLoadDatabase "O.Common/streamApp.dbd"
streamApp_registerRecordDeviceDriver

#setup the busses
drvAsynSerialPortConfigure "COM2", "/dev/ttyS1"
drvAsynSerialPortConfigure "stdout", "/dev/stdout"
drvAsynIPPortConfigure "terminal", "localhost:40000"

#load the records
dbLoadRecords "example.db"

#var streamDebug 1
iocInit
