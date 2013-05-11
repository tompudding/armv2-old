#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "armv2.h"

#define LOG(...) printf(__VA_ARGS__)
//#define LOG(...)

armv2status_t init_armv2(armv2_t *cpu, uint32_t memsize) {
    uint32_t num_pages = 0;
    armv2status_t retval = ARMV2STATUS_OK;
    if(NULL == cpu) {
        LOG("%s error, NULL cpu\n",__func__);
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    //round memsize up to a full page
    memsize = (memsize + PAGE_MASK)&(~PAGE_MASK);
    if(memsize&PAGE_MASK) {
        LOG("Page mask erro\n");
        return ARMV2STATUS_VALUE_ERROR;
    }
    num_pages = memsize>>PAGE_SIZE_BITS;
    if(num_pages > NUM_PAGE_TABLES) {
        LOG("Serious page table error, too many requested\n");
        return ARMV2STATUS_VALUE_ERROR;
    }
    if(memsize > MAX_MEMORY) {
        LOG("%s error, request memory size(%u) too big\n",__func__,memsize);
        return ARMV2STATUS_VALUE_ERROR;
    }
    memset(cpu,0,sizeof(armv2_t));
    cpu->physical_ram = malloc(memsize);
    if(NULL == cpu->physical_ram) {
        cpu->physical_ram = NULL;
        return ARMV2STATUS_MEMORY_ERROR;
    }
    cpu->physical_ram_size = memsize;
    LOG("Have %u pages %u\n",num_pages,memsize);
    memset(cpu->physical_ram,0,memsize);
    
    //map the physical ram at 0
    //we could malloc all the page tables for it at once, but all the extra bookkeeping would 
    //be annoying
    
    for(uint32_t i=0;i<num_pages;i++) {
        page_info_t *page_info = malloc(sizeof(page_info_t));
        if(NULL == page_info) {
            retval = ARMV2STATUS_MEMORY_ERROR;
            goto cleanup;
        }
        page_info->memory = cpu->physical_ram + i*WORDS_PER_PAGE;
        LOG("Page %u memory %p\n",i,(void*)page_info->memory);
        page_info->flags |= (PERM_READ|PERM_EXECUTE|PERM_WRITE);
        if(i == 0) {
            //the first page is never writable, we'll put the boot rom there.
            page_info->flags &= (~PERM_WRITE);
        }
        cpu->page_tables[i] = page_info;
    }

    cpu->flags = FLAG_INIT;
    
    cpu->regs.r[PC] = MODE_SUP; 
    cpu->pins = 0;
    cpu->pc = -4; //hack because it gets incremented on the first loop
    

cleanup:
    if(retval != ARMV2STATUS_OK) {
        cleanup_armv2(cpu);
    }
    
    return retval;
}

armv2status_t cleanup_armv2(armv2_t *cpu) {
    LOG("ARMV2 cleanup\n");
    if(NULL == cpu) {
        return ARMV2STATUS_OK;
    }
    if(NULL != cpu->physical_ram) {
        free(cpu->physical_ram);
        cpu->physical_ram = NULL;
    }
    for(uint32_t i=0;i<NUM_PAGE_TABLES;i++) {
        if(NULL != cpu->page_tables[i]) {
            free(cpu->page_tables[i]);
            cpu->page_tables[i] = NULL;
        }
    }
    return ARMV2STATUS_OK;
}

armv2status_t load_rom(armv2_t *cpu, const char *filename) {
    FILE *f = NULL;
    ssize_t read_bytes = 0;
    armv2status_t retval = ARMV2STATUS_OK;
    if(NULL == cpu) {
        return ARMV2STATUS_OK;
    }
    if(!(cpu->flags&FLAG_INIT)) {
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    if(NULL == cpu->page_tables[0] || NULL == cpu->page_tables[0]->memory) {
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    f = fopen(filename,"rb");
    if(NULL == f) {
        LOG("Error opening %s\n",filename);
        return ARMV2STATUS_IO_ERROR;
    }
    
    read_bytes = fread(cpu->page_tables[0]->memory,1,PAGE_SIZE,f);
    //24 is the bare minimum for a rom, as the vectors go up to 0x20, and then you need at least one instruction
    if(read_bytes < 0x24) {
        LOG("Error");
        retval = ARMV2STATUS_IO_ERROR;
        goto close_file;
    }

close_file:
    fclose(f);
    return retval;
}

armv2exception_t ALUInstruction                         (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t MultiplyInstruction                    (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t SwapInstruction                        (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t SingleDataTransferInstruction          (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t BranchInstruction                      (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    if((instruction>>24&1)) {
        cpu->regs.r[LR] = cpu->pc-4;
    }
    cpu->pc = cpu->pc + 8 + ((instruction&0xffffff)<<2);
    
    return EXCEPT_NONE;
}
armv2exception_t MultiDataTransferInstruction           (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t SoftwareInterruptInstruction           (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t CoprocessorDataTransferInstruction     (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t CoprocessorRegisterTransferInstruction (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}
armv2exception_t CoprocessorDataOperationInstruction    (armv2_t *cpu,uint32_t instruction)
{
    LOG("%s\n",__func__);
    return EXCEPT_NONE;
}


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
                cpu->regs.r[R14_F] = cpu->regs.r[PC];
                SETMODE(cpu,MODE_FIQ);
                SETFLAG(cpu,F);
                SETFLAG(cpu,I);
                cpu->pc = 0x1c-4;
                continue;
            }
        }
        if(FLAG_CLEAR(cpu,I)) {
            if(PIN_ON(cpu,I)) {
                //crumbs, time to do an FIQ!
                cpu->regs.r[R14_I] = cpu->regs.r[PC];
                SETMODE(cpu,MODE_IRQ);
                SETFLAG(cpu,I);
                cpu->pc = 0x18-4;
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
