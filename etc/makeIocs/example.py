#!/bin/env dls-python2.4

# setup the path
from pkg_resources import require
require("dls_dependency_tree")
require("iocbuilder")

# import modules
import iocbuilder
from dls_dependency_tree import dependency_tree

# parse the options supplied to the program
options = iocbuilder.ParseEtcArgs(architecture="linux-x86")

# Do the moduleversion calls
from iocbuilder import ModuleVersion
ModuleVersion('asyn', '4-10', use_name=True, home='/dls_sw/prod/R3.14.8.2/support')
ModuleVersion('pyDrv', '1-2', use_name=True, home='/dls_sw/prod/R3.14.8.2/support')
v = ModuleVersion("streamDevice", home="../..", use_name=False, auto_instantiate=True)

# import the builder objects
from iocbuilder.modules import *

# Create the simulation
streamDeviceSim = pyDrv.serial_sim_instance(name='streamDeviceSim', pyCls='streamDevice', module='streamDevice_sim', IPPort=8100, rpc=9001, debug=9010)
streamDeviceAsyn = asyn.AsynIP(name='streamDeviceAsyn', port='172.23.111.180:7001', simulation=streamDeviceSim)

# Create a streamDevice protocol
proto_file = streamDevice.ProtocolFile("../../data/test.proto", module=v)
proto = streamDevice.streamProtocol(streamDeviceAsyn, proto_file)

P = "TESTSTREAM:"      
def gen(var):
    # normal read
    r = proto.ai(P+var+":RBV", "readint", var, SCAN="1 second")
    proto.ao(P+var, "writeint", var, FLNK=r)        
    proto.ai(P+var+":RBVA", "read%s" % var, var, SCAN="I/O Intr")    

gen("A")
gen("B")

for ext in ("ao", "stringout", "bo", "longout"):
    proto.__dict__[ext](P+ext, "reseed")
                
# write the IOC
iocbuilder.WriteNamedIoc(options.iocpath, options.iocname, substitute_boot=True)
