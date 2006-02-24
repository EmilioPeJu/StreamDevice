#!O.linux-x86/streamApp
epicsEnvSet STREAM_PROTOCOL_PATH .:protocols:../protocols/
dbLoadDatabase O.Common/streamApp.dbd
streamApp_registerRecordDeviceDriver
dbLoadRecords example.db
drvAsynSerialPortConfigure COM2 /dev/ttyS1
drvAsynSerialPortConfigure stdout /dev/stdout
drvAsynIPPortConfigure terminal localhost:40000
#var streamDebug 1
iocInit
var streamDebug 1
