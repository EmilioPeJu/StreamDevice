# 13/5/03 vxWorks/EPICS startup file, minimal file intended just as an example of streamDevice usage
# This IOC is configured for :
#       Hytec IP Carrier 8002 card in slot 7 - 
#       Hytec 8515 Octal serial module in slot B

#streamTop = "/home/pho/diamond/streamDevice"
streamTop  = "/home/diamond/R3.13.9/work/support/streamDevice/Rx-y"
diamondTop = "/home/diamond/R3.13.9/work/support/superTop/Rx-y"

###########################################################

cd diamondTop
cd "bin/ppc604" 
ld < iocCore
ld < seq
ld < baseLib
ld < ipacLib
ld < drvHy8515.o

cd streamTop
cd "bin/ppc604" 
ld < streamLib
ld < streamTty.o

###########################################################

cd diamondTop
cd "dbd" 
dbLoadDatabase "baseApp.dbd"
dbLoadDatabase "drvIpac.dbd" 

cd streamTop
cd "dbd" 
dbLoadDatabase "stream.dbd" 

###########################################################
# Configure a Hytec 8002 carrier VME slot 7
#
#                        vmeslotnum, IPintlevel, HSintnum
IPAC7 = ipacEXTAddCarrier (&EXTHy8002, "7 2 11")

###########################################################
# Hytec 8515 IPOctal serial module in slot B on the IP carrier card. 
#
# Configure module on carrier 7, slot 1
# Params are : 
#	cardnum, 
#	vmeslotnum, 
#	ipslotnum, 
#	vectornum, 
#	intdelay (-ve => FIFO interrupt), 
#	halfduplexmode, 
#	delay845
#
MOD71 = Hy8515Configure (71, IPAC7, 1, 5, -32, 0, 0)

# Create devices
# Params are :
#	name
#	card number
#	port number
#	read buffer size
#	write buffer size
#
PORT710 = tyHYOctalDevCreate("/ty/71/0", MOD71, 0, 2500, 250)
PORT711 = tyHYOctalDevCreate("/ty/71/1", MOD71, 1, 2500, 250)
PORT712 = tyHYOctalDevCreate("/ty/71/2", MOD71, 2, 2500, 250)
PORT713 = tyHYOctalDevCreate("/ty/71/3", MOD71, 3, 2500, 250)
PORT714 = tyHYOctalDevCreate("/ty/71/4", MOD71, 4, 2500, 250)
PORT715 = tyHYOctalDevCreate("/ty/71/5", MOD71, 5, 2500, 250)
PORT716 = tyHYOctalDevCreate("/ty/71/6", MOD71, 6, 2500, 250)
PORT717 = tyHYOctalDevCreate("/ty/71/7", MOD71, 7, 2500, 250)

# Configure ports
# baud, parity(N/E/O), stop, bits, flow control(N/H)
#
tyHYOctalConfig (PORT710, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT711, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT712, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT713, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT714, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT715, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT716, 9600, 'E', 1, 8, 'N')
tyHYOctalConfig (PORT717, 9600, 'E', 1, 8, 'N')

###########################################################
# Configure stream device
#
STREAM_PROTOCOL_DIR = malloc (100)
strcpy (STREAM_PROTOCOL_DIR, diamondTop)
strcat (STREAM_PROTOCOL_DIR, "/protocol")

ty_71_0_streamBus = "Tty"
ty_71_1_streamBus = "Tty"
ty_71_2_streamBus = "Tty"
ty_71_3_streamBus = "Tty"
ty_71_4_streamBus = "Tty"
ty_71_5_streamBus = "Tty"
ty_71_6_streamBus = "Tty"
ty_71_7_streamBus = "Tty"

STREAM_PROTOCOL_DIR = "/home/diamond/work/support/streamDeviceConfig/streamProtocols"


###########################################################


iocInit

#############################################
