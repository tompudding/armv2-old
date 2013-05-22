FP                   = 12
SP                   = 13
LR                   = 14
PC                   = 15
SP_S                 = 16
LR_S                 = 17
SP_I                 = 18
LR_I                 = 19
R8_F                 = 20
R9_F                 = 21
R10_F                = 22
R11_F                = 23
FP_F                 = 24
SP_F                 = 25
LR_F                 = 26
R13_S                = 16
R14_S                = 17
R13_I                = 18
R14_I                = 19
R12_F                = 24
R13_F                = 25
R14_F                = 26

NUM_EFFECTIVE_REGS   = (16)
PAGE_SIZE_BITS       = (12)
PAGE_SIZE            = (1<<PAGE_SIZE_BITS)
PAGE_MASK            = (PAGE_SIZE-1)
NUM_PAGE_TABLES      = (1<<(26 - PAGE_SIZE_BITS))
WORDS_PER_PAGE       = (1<<(PAGE_SIZE_BITS-2))
MAX_MEMORY           = (1<<26)

from libc.stdint cimport uint32_t, int64_t

cdef extern from "armv2.h":
    ctypedef enum armv2status_t:
        ARMV2STATUS_OK
        ARMV2STATUS_INVALID_CPUSTATE
        ARMV2STATUS_MEMORY_ERROR
        ARMV2STATUS_VALUE_ERROR
        ARMV2STATUS_IO_ERROR
        ARMV2STATUS_BREAKPOINT

    enum: NUMREGS 
    enum: NUM_EFFECTIVE_REGS
    ctypedef struct regs_t:
        uint32_t actual[NUMREGS]
        uint32_t *effective[NUM_EFFECTIVE_REGS]

# typedef void (*access_callback_t)(uint32_t);

# typedef struct {
#     uint32_t *memory;
#     access_callback_t read_callback;
#     access_callback_t write_callback;
#     uint32_t flags;
# } page_info_t;

# typedef struct {
#     regs_t regs; //storage for all the registers
    
#     uint32_t *physical_ram;
#     uint32_t physical_ram_size;
#     page_info_t *page_tables[NUM_PAGE_TABLES];
#     exception_handler_t exception_handlers[EXCEPT_MAX];
#     //the pc is broken out for efficiency, when needed accessed r15 is updated from them
#     uint32_t pc;
#     //the flags are about the processor(like initialised), not part of it
#     uint32_t flags;
#     //simulating hardware pins:
#     uint32_t pins;
# } armv2_t;


    # armv2status_t init_armv2(armv2_t *cpu, uint32_t memsize)
    # armv2status_t load_rom(armv2_t *cpu, const char *filename)
    # armv2status_t cleanup_armv2(armv2_t *cpu)
    # armv2status_t run_armv2(armv2_t *cpu, int32_t instructions)
