#include "armv2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

armv2status_t init(armv2_t *cpu, uint32_t memsize) {
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
    //Add a page to the physical RAM for the interrupt page
    cpu->physical_ram = malloc(memsize + PAGE_SIZE);
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
        if(i == num_pages) {
            //this is the magic interrupt status page and it goes at the end
            page_info->flags &= (~PERM_WRITE);
            cpu->page_tables[INTERRUPT_PAGE_NUM] = page_info;
        }
        else {
            cpu->page_tables[i] = page_info;
        }
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

armv2status_t add_hardware(armv2_t *cpu, hardware_device_t *device) {
    if(NULL == cpu || NULL == device || !CPU_INITIALISED(cpu)) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    if(cpu->num_hardware_devices >= HW_DEVICES_MAX) {
        return ARMV2STATUS_MAX_HW;
    }
    //There's space, so let's add it
    cpu->hardware_devices[cpu->num_hardware_devices++] = device;
    //initialise the interrupt address herre

    return ARMV2STATUS_OK;
}

armv2status_t map_memory(armv2_t *cpu, uint32_t device_num, uint32_t start, uint32_t end) {
    uint32_t page_pos    = 0;
    uint32_t page_start  = PAGEOF(start);
    uint32_t page_end    = PAGEOF(end);
    hardware_mapping_t hw_mapping = {0};
    if(NULL == cpu || end <= start) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    if(device_num >= cpu->num_hardware_devices) {
        return ARMV2STATUS_NO_SUCH_DEVICE;
    }
    if(NULL == cpu->hardware_devices[device_num]) {
        return ARMV2STATUS_INVALID_CPUSTATE;
    }
    if(page_start == 0                  ||
       page_end == 0                    ||
       page_start >= NUM_PAGE_TABLES    ||
       page_end >= NUM_PAGE_TABLES      ||
       page_start == INTERRUPT_PAGE_NUM ||
       page_end == INTERRUPT_PAGE_NUM ) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    //First we need to know if all of the requested memory is available for mapping. That means
    //it must not have been mapped already, and it may not be the zero page, nor the IRQ page
    for(page_pos = page_start; page_pos <= page_end; page_pos++) {
        page_info_t *page;
        if(page_pos >= NUM_PAGE_TABLES || page_pos == INTERRUPT_PAGE_NUM) {
            return ARMV2STATUS_MEMORY_ERROR;
        }
        page = cpu->page_tables[page_pos];
        if(page == NULL) {
            return ARMV2STATUS_MEMORY_ERROR;
        }
        if(page->read_callback || page->write_callback) {
            return ARMV2STATUS_ALREADY_MAPPED;
        }
    }
    hw_mapping.device = cpu->hardware_devices[device_num];
    //If we get here then the entire range is free, so we can go ahead and fill it in
    for(page_pos = page_start; page_pos <= page_end; page_pos++) {
        page_info_t *page    = cpu->page_tables[page_pos];
        //Already checked everything's OK, and we're single threaded, so this should be ok I think...
        page->read_callback  = hw_mapping.device->read_callback;
        page->write_callback = hw_mapping.device->write_callback;
    }

    hw_mapping.start = start;
    hw_mapping.end   = end;
    hw_mapping.flags = 0; //maybe use these later
    add_mapping(&(cpu->hw_mappings),&hw_mapping);

    return ARMV2STATUS_OK;
}

armv2status_t add_mapping(hardware_mapping_t **head,hardware_mapping_t *item) {
    if(NULL == head) {
        return ARMV2STATUS_INVALID_ARGS;
    }
    item->next = *head;
    *head = item;
    return ARMV2STATUS_OK;
}
