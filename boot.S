.syntax divided
.code 32
reset:
        B dummy_handler
illegal_instruction:
        B dummy_handler
swi:
        B dummy_handler
prefetch_abort:
        B dummy_handler
data_abort:
        B dummy_handler
address_exception:
        B dummy_handler
IRQ:
        B dummy_handler
FIQ:
        B dummy_handler

dummy_handler:
        ADD R0,R0,#1
        LDR R1,=0x123456
        CMP R0,R1
        BGE loop_complete
        B dummy_handler
loop_complete:
        B loop_complete
