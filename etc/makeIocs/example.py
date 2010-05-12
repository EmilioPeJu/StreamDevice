#!/bin/env python2.6

# setup the path
from pkg_resources import require
require("dls_dependency_tree")
require("iocbuilder")
import sys
#sys.path.append("/dls_sw/work/common/python/iocbuilder")

# import modules
import iocbuilder
from dls_dependency_tree import dependency_tree

# parse the options supplied to the program
options, args = iocbuilder.ParseEtcArgs(architecture="linux-x86")
# parse configure/RELEASE and configure iocbuilder
iocbuilder.ParseAndConfigure(options, dependency_tree)

# import the builder objects
from iocbuilder.modules import *

# Create the simulation
streamDeviceAsyn = asyn.AsynIP(name='streamDeviceAsyn', port="localhost:8100")

# Create a streamDevice protocol
proto_file = streamDevice.ProtocolFile("../../data/test.proto", module=streamDevice.ProtocolFile.ModuleVersion)
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
    rec = proto.__dict__[ext]
    rec(P+ext, "reseed")
                
# write the IOC
iocbuilder.WriteNamedIoc(options.iocpath, options.iocname, substitute_boot=True)
