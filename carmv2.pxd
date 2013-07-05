from libc.stdint cimport uint32_t, int64_t, int32_t

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
    enum: FP
    enum: SP
    enum: LR
    enum: PC
    enum: SP_S
    enum: LR_S
    enum: SP_I
    enum: LR_I
    enum: R8_F 
    enum: R9_F 
    enum: R10_F
    enum: R11_F
    enum: FP_F 
    enum: SP_F 
    enum: LR_F 
    enum: R13_S
    enum: R14_S
    enum: R13_I
    enum: R14_I
    enum: R12_F
    enum: R13_F
    enum: R14_F
    enum: PAGE_SIZE_BITS
    enum: PAGE_SIZE     
    enum: PAGE_MASK     
    enum: NUM_PAGE_TABLES
    enum: WORDS_PER_PAGE 
    enum: MAX_MEMORY     
    enum: SWI_BREAKPOINT

    ctypedef enum:
        EXCEPT_RST                   
        EXCEPT_UNDEFINED_INSTRUCTION 
        EXCEPT_SOFTWARE_INTERRUPT    
        EXCEPT_PREFETCH_ABORT        
        EXCEPT_DATA_ABORT            
        EXCEPT_ADDRESS               
        EXCEPT_IRQ                   
        EXCEPT_FIQ                   
        EXCEPT_NONE                  
        EXCEPT_BREAKPOINT            
        EXCEPT_MAX

    ctypedef struct exception_handler_t:
        uint32_t mode
        uint32_t pc
        uint32_t flags
        uint32_t save_reg

    ctypedef enum armv2status_t:
        ARMV2STATUS_OK
        ARMV2STATUS_INVALID_CPUSTATE,
        ARMV2STATUS_MEMORY_ERROR,
        ARMV2STATUS_VALUE_ERROR,
        ARMV2STATUS_IO_ERROR,
        ARMV2STATUS_BREAKPOINT

    ctypedef struct regs_t:
        uint32_t actual[NUMREGS]
        uint32_t *effective[NUM_EFFECTIVE_REGS]

    ctypedef void (*access_callback_t)(uint32_t)

    ctypedef struct page_info_t:
        uint32_t *memory
        access_callback_t read_callback
        access_callback_t write_callback
        uint32_t flags

    ctypedef struct armv2_t:
        regs_t regs
        uint32_t *physical_ram
        uint32_t physical_ram_size
        page_info_t *page_tables[NUM_PAGE_TABLES]
        exception_handler_t exception_handlers[EXCEPT_MAX]
        uint32_t pc
        uint32_t flags
        uint32_t pins

    ctypedef struct hardware_device_t:
        uint32_t device_id
        uint32_t interrupt_flag_addr
        access_callback_t read_callback
        access_callback_t write_callback

    armv2status_t init(armv2_t *cpu, uint32_t memsize)
    armv2status_t load_rom(armv2_t *cpu, const char *filename)
    armv2status_t cleanup_armv2(armv2_t *cpu)
    armv2status_t run_armv2(armv2_t *cpu, int32_t instructions)
    armv2status_t add_hardware(armv2_t *cpu, hardware_device_t *device)
