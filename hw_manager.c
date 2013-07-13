#include "armv2.h"
#include <stdio.h>
#include <stdarg.h>
#include "hw_manager.h"

armv2status_t HwManagerDataOperation(armv2_t *cpu, uint32_t crm, uint32_t aux, uint32_t crd, uint32_t crn, uint32_t opcode) {
    if(NULL == cpu               || 
       crd >= HW_MANAGER_NUMREGS ||
       crm >= HW_MANAGER_NUMREGS ||
       crn >= HW_MANAGER_NUMREGS ||
       aux >= HW_MANAGER_NUMREGS) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    switch((hw_manager_opcode_t)opcode) {
    case NUM_DEVICES:
        /* put the current number of devices in to cr0 */
        cpu->hardware_manager.regs[crd] = cpu->num_hardware_devices;
        return ARMV2STATUS_OK;
    case MAP_MEMORY:
        /* Assign hardware device stored in crd the memory from crm up to crn. Store error code in aux */
        {
            uint32_t device_num  = cpu->hardware_manager.regs[crd];
            uint32_t mem_start   = cpu->hardware_manager.regs[crm];
            uint32_t mem_end     = cpu->hardware_manager.regs[crn];
            armv2status_t result = map_memory(cpu,device_num,mem_start,mem_end);
            //FIXME: set aux here on error
            return result;
        }
    case GET_DEVICE_ID:
        /* Get the device id of a given device and store it in CR0 */
        /* FIXME: store the result in a chosen register rather than just using cr0 */
        {
            uint32_t device_num = cpu->hardware_manager.regs[crd];
            if(device_num >= cpu->num_hardware_devices) {
                return ARMV2STATUS_NO_SUCH_DEVICE;
            }
            hardware_device_t *device = cpu->hardware_devices[device_num];
            if(NULL == device) {
                return ARMV2STATUS_NO_SUCH_DEVICE;
            }
            cpu->hardware_manager.regs[0] = device->device_id;
            return ARMV2STATUS_OK;
        }
    default:
        return ARMV2STATUS_UNKNOWN_OPCODE;
    }
    return ARMV2STATUS_UNIVERSE_BROKEN;
}

armv2status_t HwManagerRegisterTransfer(armv2_t *cpu, uint32_t crm, uint32_t aux, uint32_t rd, uint32_t crn, uint32_t opcode) {
    int load = opcode&1;
    opcode >>= 1;
    if(NULL == cpu) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    if(crn >= HW_MANAGER_NUMREGS) {
        return ARMV2STATUS_INVALID_ARGS;
    }

    switch((hw_register_opcode_t)opcode) {
    case MOV_REGISTER:
        /* Move to / from a coprocessor register */
        if(load) {
            uint32_t value = cpu->hardware_manager.regs[crn];
            if(rd == 15) {
                //only set the flags
                SETPSR(cpu,value);
            }
            else {
                GETREG(cpu,rd) = value;
            }
        }
        else {
            cpu->hardware_manager.regs[crn] = GETREG(cpu,rd);
        }
        return ARMV2STATUS_OK;
    default:
        return ARMV2STATUS_UNKNOWN_OPCODE;
    }
    return ARMV2STATUS_UNIVERSE_BROKEN;
}
