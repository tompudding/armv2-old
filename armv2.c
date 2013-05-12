#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "armv2.h"


armv2status_t run_armv2(armv2_t *cpu) {
    uint32_t running = 1;
    //for(running=1;running;cpu->pc = (cpu->pc+4)&0x3ffffff) {
    while(running) {
        cpu->pc = (cpu->pc+4)&0x3ffffff;
        //check if PC is valid
        SETPC(cpu,cpu->pc + 8);

        //Before we do anything, we check to see if we need to do and FIQ or an IRQ
        if(FLAG_CLEAR(cpu,F)) {
            if(PIN_ON(cpu,F)) {
                //crumbs, time to do an FIQ!
                cpu->regs.actual[R14_F] = cpu->regs.actual[PC];
                SETMODE(cpu,MODE_FIQ);
                SETFLAG(cpu,F);
                SETFLAG(cpu,I);
                for(uint32_t i=8;i<15;i++) {
                    cpu->regs.effective[i] = &cpu->regs.actual[R8_F+(i-8)];
                }
                cpu->pc = 0x1c-4;
                continue;
            }
        }
        if(FLAG_CLEAR(cpu,I)) {
            if(PIN_ON(cpu,I)) {
                //crumbs, time to do an FIQ!
                cpu->regs.actual[R14_I] = cpu->regs.actual[PC];
                SETMODE(cpu,MODE_IRQ);
                SETFLAG(cpu,I);
                cpu->pc = 0x18-4;
                for(uint32_t i=13;i<15;i++) {
                    cpu->regs.effective[i] = &cpu->regs.actual[R13_I+(i-13)];
                }
                continue;
            }
        }
        
        if(cpu->page_tables[PAGEOF(cpu->pc)] == NULL) {
            //Trying to execute an unmapped page!
            //some sort of exception
            exit(0);
        }

        armv2exception_t exception = EXCEPT_NONE;
        uint32_t instruction = DEREF(cpu,cpu->pc);
        LOG("Executing instruction %08x %08x\n",cpu->pc,instruction);
        switch(CONDITION_BITS(instruction)) {
        case COND_EQ: //Z set
            if(FLAG_SET(cpu,Z)) {
                break;
            }
            continue;
        case COND_NE: //Z clear
            if(FLAG_CLEAR(cpu,Z)) {
                break;
            }
            continue;
        case COND_CS: //C set
            if(FLAG_SET(cpu,C)) {
                break;
            }
            continue;
        case COND_CC: //C clear
            if(FLAG_CLEAR(cpu,C)) {
                break;
            }
            continue;
        case COND_MI: //N set
            if(FLAG_SET(cpu,N)) {
                break;
            }
            continue;
        case COND_PL: //N clear
            if(FLAG_CLEAR(cpu,N)) {
                break;
            }
            continue;
        case COND_VS: //V set
            if(FLAG_SET(cpu,V)) {
                break;
            }
            continue;
        case COND_VC: //V clear
            if(FLAG_CLEAR(cpu,V)) {
                continue;
            }
            break;
        case COND_HI: //C set and Z clear
            if(FLAG_SET(cpu,C) && FLAG_CLEAR(cpu,Z)) {
                break;
            }
            continue;
        case COND_LS: //C clear or Z set
            if(FLAG_CLEAR(cpu,C) || FLAG_SET(cpu,Z)) {
                break;
            }
            continue;
        case COND_GE: //N set and V set, or N clear and V clear
            if( (!!FLAG_SET(cpu,N)) == (!!FLAG_SET(cpu,V)) ) {
                break;
            }
            continue;
        case COND_LT: //N set and V clear or N clear and V set
            if( (!!FLAG_SET(cpu,N)) != (!!FLAG_SET(cpu,V)) ) {
                break;
            }
            continue;
        case COND_GT: //Z clear and either N set and V set, or N clear and V clear
            if((FLAG_CLEAR(cpu,Z) && (!!FLAG_SET(cpu,N)) == (!!FLAG_SET(cpu,V)) )) {
                break;
            }
            continue;
        case COND_LE: //Z set or N set and V clear, or N clear and V set
            if((FLAG_SET(cpu,Z) || (!!FLAG_SET(cpu,N)) != (!!FLAG_SET(cpu,V)) )) {
                break;
            }
            continue;
        case COND_AL: //Always
            break;
        case COND_NV: //Never
            continue;
        }
        //We're executing the instruction
        switch((instruction>>26)&03) {
        case 0:
            //Data processing, multiply or single data swap
            if((instruction&0xf0) != 0x90) {
                //data processing instruction...
                exception = ALUInstruction(cpu,instruction);
            }
            else if(instruction&0xf00) {
                //multiply
                exception = MultiplyInstruction(cpu,instruction);
            }
            else {
                //swap
                exception = SwapInstruction(cpu,instruction);
            }
            break;
        case 1:
            //LDR or STR, or undefined
            exception = SingleDataTransferInstruction(cpu,instruction);
            break;
        case 2:
            //LDM or STM or branch
            if(instruction&0x02000000) {
                exception = BranchInstruction(cpu,instruction);
            }
            else {
                exception = MultiDataTransferInstruction(cpu,instruction);
            }
            break;
        case 3:
            //coproc functions or swi
            if((instruction&0x0f000000) == 0x0f000000) {
                exception = SoftwareInterruptInstruction(cpu,instruction);
            }
            else if((instruction&0x02000000) == 0) {
                exception = CoprocessorDataTransferInstruction(cpu,instruction);
            }
            else if(instruction&0x10) {
                exception = CoprocessorRegisterTransferInstruction(cpu,instruction);
            }
            else {
                exception = CoprocessorDataOperationInstruction(cpu,instruction);
            }
            break;
        }
        //handle the exception if there was one
        if(exception != EXCEPT_NONE) {
            LOG("Instruction exception %d\n",exception);
        }
    }
    return ARMV2STATUS_OK;
}

int main(int argc, char *argv[]) {
    armv2_t armv2;
    armv2status_t result = ARMV2STATUS_OK;

    if(ARMV2STATUS_OK != (result = init_armv2(&armv2,(1<<20)))) {
        LOG("Error %d creating\n",result);
        return result;
    }
    if(ARMV2STATUS_OK != (result = load_rom(&armv2,"boot.rom"))) {
        LOG("Error loading rom %d\n",result);
        return result;
    }
    run_armv2(&armv2);
    goto cleanup;

cleanup:
    cleanup_armv2(&armv2);
    return result;
}
