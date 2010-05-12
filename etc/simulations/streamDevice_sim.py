#!/dls_sw/tools/bin/python2.4

#from pkg_resources import require
#require("dls_serial_sim")
#from dls_serial_sim import serial_device

import sys
sys.path.append("/dls_sw/work/common/python/serial_sim")
from src import serial_device, CreateSimulation

class streamDevice(serial_device):

    Terminator = "\r\n"
    
    def __init__(self):
        self.vals = dict(A=1, B=2)
        serial_device.__init__(self)
        self.schedule(self.report,0.105)
        self.diagLevel = 2         
        self.reporting = False
        self.reseedDone = False
    
    def report(self): 
        # report on A, B
        if self.reporting:
            return "%s %s" %(self.vals["A"], self.vals["B"])
    
    def reply(self,command):   
        if command == "SYSTEM:RESEED":
            self.reseedDone = True
            return     
        if command.endswith("?"):
            # <var>? returns self.vals[<var>]
            var = command[:-1]
            val = None
        elif "=" in command:
            # <var>=<val> sets self.vals[<var>]=<val>
            var, val = command.split("=")
        else:
            # malformed command
            return "ERR001"
        if var not in self.vals:
            # unknown param
            return "ERR002"
        if val is None:
            return self.vals[var]
        else:
            try:                
                self.vals[var] = int(val)
                return "OK"
            except:
                # cannot convert to int
                return "ERR003"

if __name__=="__main__":
    CreateSimulation(streamDevice)
    raw_input()
