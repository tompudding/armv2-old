.syntax divided
.code 32

num_devices_opcode  = 0
map_memory_opcode   = 1
device_id_opcode    = 2
mov_register_opcode = 0
hw_manager          = 1
keyboard_device_id  = 0x41414141
        
reset:
        B start
illegal_instruction:
        SWI #1
swi:
        SWI #2
prefetch_abort:
        SWI #3
data_abort:
        SWI #4
address_exception:
        SWI #5
IRQ:
        SWI #6
FIQ:
        SWI #7

start:
        LDR R6,=keyboard_device_id
        /* Load the number of hardware devices into cr0 */
        CDP 1,#num_devices_opcode,CR0,CR0,CR0
        /* Put cr0 into r0 */
        MRC 1,#mov_register_opcode,R0,CR0,CR0
        /* Loop over the device numbers getting their id and comparing it to the keyboard id*/
        MOV R1,#0
hw_id_loop:        
        CMP R1,R0
        BGE hw_loop_done
        /* Load the current index (R1) into CR0 */
        MCR 1,#mov_register_opcode,R1,CR0,CR0
        /* Perform the get id operation */
        CDP 1,#device_id_opcode,CR0,CR0,CR0
        /* Move the output into R2 */
        MRC 1,#mov_register_opcode,R2,CR0,CR0
hw_loop_done:   
        
        /* Set CR1 to be the start value */
        /* Set CR2 to be the end value */
        /* Instruct */
        MOV SP,#0x100000
        SUB SP,SP,#4
        LDR R0,=0x80ac
        ADR LR,exit
        MOVS PC,R0
        SWP R0,R0,[R1]
exit:   
        SWI #8
