#include "armv2.h"
#include <stdio.h>
#include <stdarg.h>
#include "hw_manager.h"

armv2status_t HwManagerDataOperation(armv2_t *cpu, uint32_t crm, uint32_t aux, uint32_t crd, uint32_t crn, uint32_t opcode) {
    if(NULL == cpu) {
        return ARMV2STATUS_INVALID_ARGS;
    }

    switch((hw_manager_opcode_t)opcode) {
    case NUM_DEVICES:
        /* put the current number of devices in to cr0 */
        cpu->hardware_manager.regs[0] = cpu->num_hardware_devices;
        return ARMV2STATUS_OK;
    default:
        return ARMV2STATUS_UNKNOWN_OPCODE;
    }
    return ARMV2STATUS_UNIVERSE_BROKEN;
}

armv2status_t HwManagerRegisterTransfer(armv2_t *cpu, uint32_t crm, uint32_t aux, uint32_t crd, uint32_t crn, uint32_t opcode) {
    if(NULL == cpu) {
        return ARMV2STATUS_INVALID_ARGS;
    }

    switch((hw_manager_opcode_t)opcode) {
    case NUM_DEVICES:
        /* put the current number of devices in to cr0 */
        cpu->hardware_manager.regs[0] = cpu->num_hardware_devices;
        return ARMV2STATUS_OK;
    default:
        return ARMV2STATUS_UNKNOWN_OPCODE;
    }
    return ARMV2STATUS_UNIVERSE_BROKEN;
}
