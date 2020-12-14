#!/dls_sw/tools/bin/python2.4

from pkg_resources import require
require('dls.autotestframework')
from dls.autotestframework import *
from cothread.catools import *

#import sys
#sys.path.append("/dls_sw/work/common/python/autotestframework")
#from src import *

from random import randrange as r

P = "TESTSTREAM:"
ip = 8100
rpc = 9100
debug = 10100

class StreamDeviceTestSuite(TestSuite):
    def createTests(self):
        Target( "simulation", self,[
            ModuleEntity('streamDevice'),
            IocEntity('ioc', directory='iocs/example_sim', bootCmd='bin/linux-x86/stexample.sh'),
            SimulationEntity('sim', runCmd='data/streamDevice_sim.py -i %d -r %d -d %d'%(ip, rpc, debug), rpcPort=rpc),
            EpicsDbEntity('db', directory='iocs/example_sim/db', fileName='example_expanded.db')])
        CaseNoReport(self)
#        CaseReport(self)
        for ext in ("ao", "stringout", "bo", "longout"):
            CaseReseed(ext, self)
        

class StreamDeviceTestCase( TestCase ):
                
    # Convenience function to set a PV and verify it has taken effect in the module.
    # The set PV will always be verified after put and if a readback PV has been specified
    # in rbvpv it's value will be verified against either the set value or rbvvalue if it
    # has been defined.
    # The optional delay parameter can be used between the putPv and the readback/verify.
    def putPvAndVerify(self, setpv, value, rbvpv = None, rbvvalue = None, validrange=None, delay=None, delta=0.0):
        self.putPv( setpv, value )
        if delay!=None:
            self.sleep(delay)
        self.verifyPv( setpv, value )
        if ( rbvpv ):
            verifyvalue = value
            if rbvvalue!=None:
                verifyvalue = rbvvalue
            #print "Verify PV: %s against value: %s"%(rbvpv, str(verifyvalue))
            if type(verifyvalue)==float:
                self.verifyPvFloat( rbvpv, verifyvalue, delta )
            else:
                self.verifyPv( rbvpv, verifyvalue )
                
    def gather_values(self, t, *pvs):
        rets = [[] for pv in pvs]
        def f(value, index):
            rets[index].append(value)        
        subs = camonitor(pvs, f)
        self.sleep(t)
        for s in subs:
            s.close()        
        return rets

    def wait_for_errors(self):
        As, Bs = self.gather_values(10, P+"A:RBV.SEVR", P+"B:RBV.SEVR")
        for x in [x for x in As + Bs if x != 0]:
            self.fail("Failed on SEVR test with SEVR=%d" % x)
        
                        
class CaseNoReport(StreamDeviceTestCase):
    def runTest(self):
        self.simulation('sim').reporting = False
        self.sleep(2)
        self.wait_for_errors()

class CaseReport(StreamDeviceTestCase):
    def runTest(self):
        self.simulation('sim').reporting = True
        self.sleep(2)        
        self.wait_for_errors()

class CaseReseed(StreamDeviceTestCase):
    def __init__(self, ext, *args, **kwargs):
        StreamDeviceTestCase.__init__(self, *args, **kwargs)
        self.ext = ext
        
    def runTest(self):
        self.simulation('sim').reseedDone = False
        self.putPv(P+self.ext+".PROC", 1)
        self.sleep(1)
        assert self.simulation('sim').reseedDone == True

if __name__ == "__main__":
    # Create and run the test sequence
    StreamDeviceTestSuite()
