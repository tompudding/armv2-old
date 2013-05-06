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

#define PAGE_SIZE_BITS 16
#define PAGE_SIZE (1<<PAGE_SIZE_BITS)
#define PAGE_MASK (PAGE_SIZE-1)
#define NUM_PAGE_TABLES (1<<(32 - PAGE_SIZE_BITS))
#define WORDS_PER_PAGE (1<<(PAGE_SIZE_BITS-2))
#define MAX_MEMORY (1<<26)

#define PERM_READ    4
#define PERM_WRITE   2
#define PERM_EXECUTE 1

#define FLAG_INIT 1

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
    uint32_t flags;
} armv2_t;


armv2status_t init_armv2(armv2_t *cpu, uint32_t memsize);
armv2status_t load_rom(armv2_t *cpu, const char *filename);
armv2status_t cleanup_armv2(armv2_t *cpu);
