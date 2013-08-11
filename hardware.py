import armv2

class Keyboard(armv2.Device):
    id = 0x41414141
    def __init__(self):
        super(Keyboard,self).__init__()
        armv2.DebugLog('keyboard keyboard keyboard!\n')

    def readCallback(self,addr,value):
        armv2.DebugLog('keyboard reader %x %x\n' % (addr,value))
        return 0;

    def writeCallback(self,addr,value):
        armv2.DebugLog('keyboard writer %x %x\n' % (addr,value))

    
