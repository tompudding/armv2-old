#include "armv2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

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
        //LOG("Page %u memory %p\n",i,(void*)page_info->memory);
        page_info->flags |= (PERM_READ|PERM_EXECUTE|PERM_WRITE);
        if(i == 0) {
            //the first page is never writable, we'll put the boot rom there.
            page_info->flags &= (~PERM_WRITE);
        }
        cpu->page_tables[i] = page_info;
    }

    cpu->flags = FLAG_INIT;
    
    cpu->regs.actual[PC] = MODE_SUP; 
    cpu->pins = 0;
    cpu->pc = -4; //hack because it gets incremented on the first loop

    for(uint32_t i=0;i<NUM_EFFECTIVE_REGS;i++) {
        cpu->regs.effective[i] = &cpu->regs.actual[i];
    }

    //Set up the exception conditions
    for(uint32_t i=0;i<EXCEPT_NONE;i++) {
        cpu->exception_handlers[i].mode     = MODE_SUP;
        cpu->exception_handlers[i].pc       = i*4;
        cpu->exception_handlers[i].flags    = FLAG_I;
        cpu->exception_handlers[i].save_reg = LR_S;
    }
    cpu->exception_handlers[EXCEPT_IRQ].mode     = MODE_IRQ;
    cpu->exception_handlers[EXCEPT_IRQ].save_reg = LR_I;
    cpu->exception_handlers[EXCEPT_FIQ].mode     = MODE_FIQ;
    cpu->exception_handlers[EXCEPT_IRQ].save_reg = LR_I;
    cpu->exception_handlers[EXCEPT_FIQ].flags |= FLAG_F;
    cpu->exception_handlers[EXCEPT_RST].flags |= FLAG_F;

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
    struct stat st = {0};
    uint32_t page_num = 0;
    if(NULL == cpu) {
        return ARMV2STATUS_OK;
    }
    if(!(cpu->flags&FLAG_INIT)) {
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    if(NULL == cpu->page_tables[0] || NULL == cpu->page_tables[0]->memory) {
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    if(0 != stat(filename,&st)) {
        return ARMV2STATUS_IO_ERROR;
    }
    ssize_t size = st.st_size;
    if(size < 24) {
        //24 is the bare minimum for a rom, as the vectors go up to 0x20, and then you need at least one instruction
        return ARMV2STATUS_IO_ERROR;
    }
    f = fopen(filename,"rb");
    if(NULL == f) {
        LOG("Error opening %s\n",filename);
        return ARMV2STATUS_IO_ERROR;
    }
    while(size > 0) {
        read_bytes = fread(cpu->page_tables[page_num++]->memory,1,PAGE_SIZE,f);
        
        if(read_bytes < PAGE_SIZE) {
            //It's ok if it's all that's left
            
            if(read_bytes != size) {
                LOG("Error %d %zd %zd\n",page_num,read_bytes,size);
                retval = ARMV2STATUS_IO_ERROR;
                goto close_file;
            }
        }
        size -= PAGE_SIZE;
    }

close_file:
    fclose(f);
    return retval;
}
