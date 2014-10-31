#include "armv2.h"
#include <stdio.h>
#include <stdarg.h>

enum armv2_status MmuDataOperation(armv2_t *cpu,uint32_t crm, uint32_t aux, uint32_t crd, uint32_t crn, uint32_t opcode) {
    return ARMV2STATUS_OK;
}

enum armv2_status MmuRegisterTransfer      (armv2_t *cpu, uint32_t crm, uint32_t aux, uint32_t crd, uint32_t crn, uint32_t opcode) {
    return ARMV2STATUS_OK;
}
