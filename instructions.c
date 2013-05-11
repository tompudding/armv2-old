#include "armv2.h"
#include <stdio.h>

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
