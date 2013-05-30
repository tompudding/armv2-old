cimport carmv2
from libc.stdint cimport uint32_t, int64_t
from libc.stdlib cimport malloc, free

NUMREGS = carmv2.NUMREGS
NUM_EFFECTIVE_REGS = carmv2.NUM_EFFECTIVE_REGS

class Registers(object):
    mapping = {'fp' : 12,
               'sp' : 13,
               'lr' : 14,
               'pc' : 15}
    for i in xrange(NUM_EFFECTIVE_REGS):
        mapping['r%d' % i] = i
               
    def __init__(self,cpu):
        self.cpu = cpu

    def __getattr__(self,attr):
        try:
            index = self.mapping[attr]
        except KeyError:
            raise AttributeError()
        return self.cpu.getregs(index)

    def __setattr__(self,attr,value):
        try:
            index = self.mapping[attr]
        except KeyError:
            super(Registers,self).__setattr__(attr,value)
            return
        self.cpu.setregs(index,value)

    def __getitem__(self,index):
        if isinstance(index,slice):
            indices = index.indices(NUM_EFFECTIVE_REGS)
            return [self.cpu.getregs(i) for i in xrange(*indices)]
        return self.cpu.getregs(index)

    def __setitem__(self,index,value):
        if isinstance(index,slice):
            indices = index.indices(NUM_EFFECTIVE_REGS)
            for i in xrange(*indices):
                self.cpu.setregs(i,value[i])
            return
        return self.cpu.setregs(index,value)

    def __repr__(self):
        return repr(self[:])

cdef class Armv2:
    cdef carmv2.armv2_t *cpu
    cdef public regs

    def __cinit__(self, *args, **kwargs):
        self.cpu = <carmv2.armv2_t*>malloc(sizeof(carmv2.armv2_t))
        if self.cpu == NULL:
            raise MemoryError()
    
    def __dealloc__(self):
        if self.cpu != NULL:
            carmv2.cleanup_armv2(self.cpu)
            free(self.cpu)
            self.cpu = NULL

    def getregs(self,index):
        if index >= NUM_EFFECTIVE_REGS:
            raise IndexError()
        return int(self.cpu.regs.effective[index][0])

    def setregs(self,index,value):
        if index >= NUM_EFFECTIVE_REGS:
            raise IndexError()
        self.cpu.regs.effective[index][0] = value
        if index == carmv2.PC:
            self.cpu.pc = int((0xfffffffc + (value&0x3ffffffc))&0xffffffff)

    def __init__(self,size,filename = None):
        cdef carmv2.armv2status_t result
        cdef uint32_t mem = size
        result = carmv2.init(self.cpu,mem)
        self.regs = Registers(self)
        if result != carmv2.ARMV2STATUS_OK:
            raise ValueError()
        if filename != None:
            self.LoadROM(filename)

    def LoadROM(self,filename):
        result = carmv2.load_rom(self.cpu,filename)
        if result != carmv2.ARMV2STATUS_OK:
            raise ValueError()

    def Step(self,number = None):
        result = carmv2.run_armv2(self.cpu,-1 if number == None else number)
        #right now can only return OK or BREAKPOINT, but we don't care either way...
        return result

