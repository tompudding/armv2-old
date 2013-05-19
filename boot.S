.syntax divided
.code 32
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
        MOV SP,#0x100000
        SUB SP,SP,#4
        LDR R0,=0x80ac
        ADR LR,exit
        MOVS PC,R0
exit:   
        SWI #8
