cimport carmv2
from libc.stdint cimport uint32_t, int64_t
from libc.stdlib cimport malloc, free
import itertools

NUMREGS = carmv2.NUMREGS
NUM_EFFECTIVE_REGS = carmv2.NUM_EFFECTIVE_REGS
MAX_26BIT = 1<<26
SWI_BREAKPOINT = carmv2.SWI_BREAKPOINT

class CpuExceptions:
    Reset                = carmv2.EXCEPT_RST                   
    UndefinedInstruction = carmv2.EXCEPT_UNDEFINED_INSTRUCTION 
    SoftwareInterrupt    = carmv2.EXCEPT_SOFTWARE_INTERRUPT    
    PrefetchAboprt       = carmv2.EXCEPT_PREFETCH_ABORT        
    DataAbort            = carmv2.EXCEPT_DATA_ABORT            
    Address              = carmv2.EXCEPT_ADDRESS               
    Irq                  = carmv2.EXCEPT_IRQ                   
    Fiq                  = carmv2.EXCEPT_FIQ                   
    Breakpoint           = carmv2.EXCEPT_BREAKPOINT         

class Status:
    Ok              = carmv2.ARMV2STATUS_OK
    InvalidCpuState = carmv2.ARMV2STATUS_INVALID_CPUSTATE
    MemoryError     = carmv2.ARMV2STATUS_MEMORY_ERROR
    ValueError      = carmv2.ARMV2STATUS_VALUE_ERROR
    IoError         = carmv2.ARMV2STATUS_IO_ERROR
    Breakpoint      = carmv2.ARMV2STATUS_BREAKPOINT

def PAGEOF(addr):
    return addr>>carmv2.PAGE_SIZE_BITS

def INPAGE(addr):
    return addr&carmv2.PAGE_MASK

def WORDINPAGE(addr):
    return INPAGE(addr)>>2

class AccessError(Exception):
    pass

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

class ByteMemory(object):
    def __init__(self,cpu):
        self.cpu    = cpu
        self.getter = self.cpu.getbyte
        self.setter = self.cpu.setbyte

    def __getitem__(self,index):
        if isinstance(index,slice):
            indices = index.indices(MAX_26BIT)
            indices = xrange(*indices)
        else:
            indices = (index,)
        return ''.join(chr(self.getter(index)) for index in indices)

    def __setitem__(self,index,values):
        if isinstance(index,slice):
            indices = index.indices(MAX_26BIT)
            indices = xrange(*indices)
        else:
            indices = (index,)
            values  = (values,)
        try:
            for i,v in itertools.izip_longest(indices,values):
                self.setter(i,ord(v))
        except TypeError:
            raise ValueError('Wrong values sequence length')

    def __len__(self):
        return MAX_26BIT

class WordMemory(object):
    def __init__(self,cpu):
        self.cpu    = cpu
        self.getter = self.cpu.getword
        self.setter = self.cpu.setword

    def __getitem__(self,index):
        if isinstance(index,slice):
            indices = index.indices(MAX_26BIT)
            indices = xrange(*indices)
        else:
            indices = (index,)
        if len(indices) == 1:
            return self.getter(index)
        else:
            return [self.getter(index) for index in indices]

    def __setitem__(self,index,values):
        if isinstance(index,slice):
            indices = index.indices(MAX_26BIT)
            indices = xrange(*indices)
        else:
            indices = (index,)
            values  = (values,)
        try:
            for i,v in itertools.izip_longest(indices,values):
                self.setter(i,v)
        except TypeError:
            raise ValueError('Wrong values sequence length')

    def __len__(self):
        return MAX_26BIT>>2


cdef class Device:
    id = None
    cdef carmv2.hardware_device_t *cdevice
    
    def __cinit__(self, *args, **kwargs):
        self.cdevice = <carmv2.hardware_device_t*>malloc(sizeof(carmv2.hardware_device_t))
        self.cdevice.device_id = self.id
        self.cdevice.read_callback = NULL;
        self.cdevice.write_callback = NULL;
        if self.cdevice == NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self.cdevice != NULL:
            free(self.cdevice)

    def __init__(self):
        self.attached_cpu = []

    def __del__(self):
        for cpu in self.attached_cpu:
            cpu.RemoveDevice(self)

    cdef carmv2.hardware_device_t *GetDevice(self):
        return self.cdevice

cdef class Armv2:
    cdef carmv2.armv2_t *cpu
    cdef public regs
    cdef public mem
    cdef public memw
    cdef public memsize
    cdef public hardware

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

    def getbyte(self,addr):
        cdef uint32_t word = self.getword(addr)
        cdef uint32_t b = (addr&3)<<3
        return (word>>b)&0xff;

    def setbyte(self,addr,value):
        cdef uint32_t word = self.getword(addr)
        cdef uint32_t b = (addr&3)<<3
        cdef uint32_t mask = 0xff<<b
        cdef uint32_t new_word = (word&(~mask)) | ((value&0xff)<<b)
        self.setword(addr,new_word)

    def getword(self,addr):
        if addr >= MAX_26BIT:
            raise IndexError()

        cdef carmv2.page_info_t *page = self.cpu.page_tables[PAGEOF(addr)]
        if NULL == page:
            #raise AccessError()
            return 0

        return page.memory[WORDINPAGE(addr)]

    def setword(self,addr,value):
        if addr >= MAX_26BIT:
            raise IndexError()

        cdef carmv2.page_info_t *page = self.cpu.page_tables[PAGEOF(addr)]
        if NULL == page:
            raise AccessError()

        page.memory[WORDINPAGE(addr)] = int(value)

    @property
    def pc(self):
        #The first thing the run loop does is add 4 to PC, so PC is effectively 4 greater than 
        #it appears to be
        return self.cpu.pc + 4

    @property
    def mode(self):
        return self.regs.pc&3

    def __init__(self,size,filename = None):
        cdef carmv2.armv2status_t result
        cdef uint32_t mem = size
        result = carmv2.init(self.cpu,mem)
        self.regs = Registers(self)
        self.memsize = size
        self.mem  = ByteMemory(self)
        self.memw = WordMemory(self)
        self.hardware = []
        if result != carmv2.ARMV2STATUS_OK:
            raise ValueError()
        if filename != None:
            self.LoadROM(filename)

    def LoadROM(self,filename):
        result = carmv2.load_rom(self.cpu,filename)
        if result != carmv2.ARMV2STATUS_OK:
            raise ValueError()

    def Step(self,number = None):
        cdef uint32_t result = carmv2.run_armv2(self.cpu,-1 if number == None else number)
        #right now can only return OK or BREAKPOINT, but we don't care either way...
        return result

    def AddHardware(self,Device device):
        result = carmv2.add_hardware(self.cpu,device.cdevice)
        if result != carmv2.ARMV2STATUS_OK:
            raise ValueError
        self.hardware.append(device)

