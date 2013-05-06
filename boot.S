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
        B dummy_handler
        