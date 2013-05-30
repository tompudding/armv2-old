cimport carmv2
from libc.stdint cimport uint32_t, int64_t
from libc.stdlib cimport malloc, free

NUMREGS = carmv2.NUMREGS

cdef class Armv2:
    cdef carmv2.armv2_t *cpu

    def __cinit__(self, *args, **kwargs):
        self.cpu = <carmv2.armv2_t*>malloc(sizeof(carmv2.armv2_t))
        if self.cpu == NULL:
            raise MemoryError()
    
    def __dealloc__(self):
        if self.cpu != NULL:
            carmv2.cleanup_armv2(self.cpu)
            free(self.cpu)
            self.cpu = NULL

    def __init__(self,size,filename = None):
        cdef carmv2.armv2status_t result
        cdef uint32_t mem = size
        result = carmv2.init(self.cpu,mem)
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

