#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "armv2.h"

armv2status_t run_armv2(armv2_t *cpu, int32_t instructions) {
    uint32_t running = 1;
    //for(running=1;running;cpu->pc = (cpu->pc+4)&0x3ffffff) {
    //instructions of -1 means run forever
    while(running) {
        if(instructions == 0) {
            return ARMV2STATUS_OK;
        }
        if(instructions > 0) {
            instructions--;
        }
        armv2exception_t exception = EXCEPT_NONE;
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
            exception = EXCEPT_PREFETCH_ABORT;
            goto handle_exception;
        }

        
        uint32_t instruction = DEREF(cpu,cpu->pc);
        LOG("%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x : %08x\n",
            GETREG(cpu,0),
            GETREG(cpu,1),
            GETREG(cpu,2),
            GETREG(cpu,3),
            GETREG(cpu,4),
            GETREG(cpu,5),
            GETREG(cpu,6),
            GETREG(cpu,7),
            GETREG(cpu,8),
            GETREG(cpu,9),
            GETREG(cpu,10),
            GETREG(cpu,11),
            GETREG(cpu,12),
            GETREG(cpu,13),
            GETREG(cpu,14),
            GETREG(cpu,15),
            instruction);
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
        instruction_handler_t handler = NULL;
        //We're executing the instruction
        switch((instruction>>26)&03) {
        case 0:
            //Data processing, multiply or single data swap
            if((instruction&0xf0) != 0x90) {
                //data processing instruction...
                handler = ALUInstruction;
            }
            else if(instruction&0xf00) {
                //multiply
                handler = MultiplyInstruction;
            }
            else {
                //swap
                handler = SwapInstruction;
            }
            break;
        case 1:
            //LDR or STR, or undefined
            handler = SingleDataTransferInstruction;
            break;
        case 2:
            //LDM or STM or branch
            if(instruction&0x02000000) {
                handler = BranchInstruction;
            }
            else {
                handler = MultiDataTransferInstruction;
            }
            break;
        case 3:
            //coproc functions or swi
            if((instruction&0x0f000000) == 0x0f000000) {
                handler = SoftwareInterruptInstruction;
            }
            else if((instruction&0x02000000) == 0) {
                handler = CoprocessorDataTransferInstruction;
            }
            else if(instruction&0x10) {
                handler = CoprocessorRegisterTransferInstruction;
            }
            else {
                handler = CoprocessorDataOperationInstruction;
            }
            break;
        }
        exception = handler(cpu,instruction);
        //handle the exception if there was one
    handle_exception:
        if(exception != EXCEPT_NONE) {
            //LOG("Instruction exception %d\n",exception);
            if(exception == EXCEPT_BREAKPOINT) {
                if(instructions == -1) {
                    //this means we're running forver, so treat this as an SWI
                    exception = EXCEPT_SOFTWARE_INTERRUPT;
                }
                else {
                    //This is special and means stop executing the emulator
                    return ARMV2STATUS_BREAKPOINT;
                }
            }
            exception_handler_t ex_handler = cpu->exception_handlers[exception];
            cpu->regs.actual[ex_handler.save_reg] = cpu->regs.actual[PC];
            cpu->regs.actual[PC] = ((cpu->regs.actual[PC])&0xfffffffc) | ex_handler.mode;
            cpu->pc = ex_handler.pc-4;
        }
    }
    return ARMV2STATUS_OK;
}
