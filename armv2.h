#include <stdint.h>

#define FP 12
#define SP 13
#define LR 14
#define PC 15
#define SP_S 16
#define LR_S 17
#define SP_I 18
#define LR_I 19
#define R8_F 20
#define R9_F 21
#define R10_F 22
#define R11_F 23
#define FP_F  24
#define SP_F  25
#define LR_F  26

#define R13_S 16
#define R14_S 17
#define R13_I 18
#define R14_I 19
#define R12_F 24
#define R13_F 25
#define R14_F 26

#define NUMREGS 27

#define PAGE_SIZE_BITS 12
#define PAGE_SIZE (1<<PAGE_SIZE_BITS)
#define PAGE_MASK (PAGE_SIZE-1)
#define NUM_PAGE_TABLES (1<<(26 - PAGE_SIZE_BITS))
#define WORDS_PER_PAGE (1<<(PAGE_SIZE_BITS-2))
#define MAX_MEMORY (1<<26)

#define PAGEOF(addr) ((addr)>>PAGE_SIZE_BITS)
#define INPAGE(addr) ((addr)&PAGE_MASK)
#define WORDINPAGE(addr) (INPAGE(addr)>>2)
#define DEREF(cpu,addr) cpu->page_tables[PAGEOF(addr)]->memory[WORDINPAGE(addr)]
#define SETPC(cpu,newpc) ((cpu)->regs.r[PC] = (((cpu)->regs.r[PC]&0xfc000003) | ((newpc)&0x003fffffc)))

#define PERM_READ    4
#define PERM_WRITE   2
#define PERM_EXECUTE 1

#define FLAG_N 0x80000000
#define FLAG_Z 0x40000000
#define FLAG_C 0x20000000
#define FLAG_V 0x10000000
#define FLAG_I 0x08000000
#define FLAG_F 0x04000000

#define FLAG_SET(cpu,flag) ((cpu)->regs.r[PC]&FLAG_##flag)
#define FLAG_CLEAR(cpu,flag) (!FLAG_SET(cpu,flag))

#define COND_EQ 0x0
#define COND_NE 0x1
#define COND_CS 0x2
#define COND_CC 0x3
#define COND_MI 0x4
#define COND_PL 0x5
#define COND_VS 0x6
#define COND_VC 0x7
#define COND_HI 0x8
#define COND_LS 0x9
#define COND_GE 0xa
#define COND_LT 0xb
#define COND_GT 0xc
#define COND_LE 0xd
#define COND_AL 0xe
#define COND_NV 0xf

#define CONDITION_BITS(x) ((x)>>28)

#define MODE_USR 0
#define MODE_FIQ 1
#define MODE_IRQ 2
#define MODE_SUP 3

#define FLAG_INIT 1

typedef enum {
    EXCEPT_RST                   = 0,
    EXCEPT_UNDEFINED_INSTRUCTION = 1,
    EXCEPT_SOFTWARE_INTERRUPT    = 2,
    EXCEPT_PREFETCH_ABORT        = 3,
    EXCEPT_DATA_ABORT            = 4,
    EXCEPT_ADDRESS               = 5,
    EXCEPT_IRQ                   = 6,
    EXCEPT_FIQ                   = 7,
    EXCEPT_NONE                  = 8,
} armv2exception_t;

typedef enum {
    ARMV2STATUS_OK = 0,
    ARMV2STATUS_INVALID_CPUSTATE,
    ARMV2STATUS_MEMORY_ERROR,
    ARMV2STATUS_VALUE_ERROR,
    ARMV2STATUS_IO_ERROR,
} armv2status_t;

typedef struct {
    uint32_t r[NUMREGS];
} regs_t;

typedef void (*access_callback_t)(uint32_t);

typedef struct {
    uint32_t *memory;
    access_callback_t read_callback;
    access_callback_t write_callback;
    uint32_t flags;
} page_info_t;

typedef struct {
    regs_t regs;
    uint32_t *physical_ram;
    uint32_t physical_ram_size;
    page_info_t *page_tables[NUM_PAGE_TABLES];
    //the pc is broken out for efficiency, when needed accessed r15 is updated from them
    uint32_t pc;
    //the flags are about the processor(like initialised), not part of it
    uint32_t flags;
} armv2_t;


armv2status_t init_armv2(armv2_t *cpu, uint32_t memsize);
armv2status_t load_rom(armv2_t *cpu, const char *filename);
armv2status_t cleanup_armv2(armv2_t *cpu);
armv2status_t run_armv2(armv2_t *cpu);

//instruction handlers
armv2exception_t ALUInstruction                         (armv2_t *cpu,uint32_t instruction);
armv2exception_t MultiplyInstruction                    (armv2_t *cpu,uint32_t instruction);
armv2exception_t SwapInstruction                        (armv2_t *cpu,uint32_t instruction);
armv2exception_t SingleDataTransferInstruction          (armv2_t *cpu,uint32_t instruction);
armv2exception_t BranchInstruction                      (armv2_t *cpu,uint32_t instruction);
armv2exception_t MultiDataTransferInstruction           (armv2_t *cpu,uint32_t instruction);
armv2exception_t SoftwareInterruptInstruction           (armv2_t *cpu,uint32_t instruction);
armv2exception_t CoprocessorDataTransferInstruction     (armv2_t *cpu,uint32_t instruction);
armv2exception_t CoprocessorRegisterTransferInstruction (armv2_t *cpu,uint32_t instruction);
armv2exception_t CoprocessorDataOperationInstruction    (armv2_t *cpu,uint32_t instruction);
