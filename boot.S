.syntax divided
.code 32

num_devices_opcode  = 0
mov_register_opcode = 0
hw_manager          = 1
        
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
        /* Load the number of hardware devices into cr0 */
        CDP 1,#num_devices_opcode,CR0,CR0,CR0
        /* Put cr0 into r0 */
        MRC 1,#mov_register_opcode,R0,CR0,CR0
        
        MOV SP,#0x100000
        SUB SP,SP,#4
        LDR R0,=0x80ac
        ADR LR,exit
        MOVS PC,R0
        SWP R0,R0,[R1]
exit:   
        SWI #8
