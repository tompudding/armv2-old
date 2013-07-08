#ifndef __HW_MANAGER_H__
#define __HW_MANAGER_H__

#include "common.h"
#define HW_MANAGER_NUMREGS 4
#define MAP_MEMORY_ERROR_OK 0
#define MAP_MEMORY_ERROR_NO_SUCH_DEVICE 1

typedef struct {
    uint32_t regs[HW_MANAGER_NUMREGS];
} hw_manager_t;

typedef enum {
    NUM_DEVICES = 0,
    MAP_MEMORY  = 1,
} hw_manager_opcode_t;

typedef enum {
    MOV_REGISTER = 0,
} hw_register_opcode_t;
    
#endif
