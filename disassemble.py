
registerNames = [('r%d' % i) for i in xrange(13)] + ['sp','lr','pc']

class Instruction(object):
    mneumonic = 'UNK'
    args = []
    def __init__(self,addr,word):
        self.addr = addr
        self.word = word
    def ToString(self):
        return '%5s %s' % (self.mneumonic,', '.join(self.args)) 

class ALUInstruction(Instruction):
    opcodes = ['AND','EOR','SUB','RSB',
               'ADD','ADC','SBC','RSC',
               'TST','TEQ','CMP','CMN',
               'ORR','MOV','BIC','MVN']
    MOV = 0xd
    MVN = 0xf
    shiftTypes = ['LSL','LSR','ASR','ROR']
    ALU_TYPE_IMM = 0x02000000
    def __init__(self,addr,word):
        super(ALUInstruction,self).__init__(addr,word)
        opcode   = (word>>21)&0xf
        rn       = (word>>16)&0xf
        rd       = (word>>12)&0xf
        self.mneumonic = self.opcodes[opcode]
        op1 = registerNames[rn]
        rd  = registerNames[rd]
        if word & self.ALU_TYPE_IMM:
            right_rotate = (word>>7)&0x1e
            if right_rotate != 0:
                val = ((word&0xff) << (32-right_rotate)) | ((word&0xff) >> right_rotate)
            else:
                val = word&0xff
            op2 = ['#0x%x' % val]
        else:
            op2 = self.OperandShift(word&0xfff,word&0x10)
        if opcode in [self.MOV,self.MVN]:
            self.args = [rd] + op2
        else:
            self.args = [rd,op1] + op2

    def OperandShift(self,bits,type_flag):
        rm = registerNames[bits&0xf]
        shift_type = self.shiftTypes[(bits>>5)&0x3]
        if type_flag:
            #shift amount is a register
            shift = registerNames[(bits>>8)&0xf]
        else:
            shift = '#%d' % ((bits>>7)&0x1f)
        if shift_type == 'ROR' and type_flag == 0 and shift == 0:
            return [rm,'RRX']
        elif shift == 0:
            return [rm]
        else:
            #shift is nonzero
            return [rm,shift_type + ' ' + shift]

class MultiplyInstruction(Instruction):
    pass

class SwapInstruction(Instruction):
    pass

class SingleDataTransferInstruction(Instruction):
    pass

class BranchInstruction(Instruction):
    pass

class MultiDataTransferInstruction(Instruction):
    pass

class SoftwareInterruptInstruction(Instruction):
    pass

class CoprocessorDataTransferInstruction(Instruction):
    pass

class CoprocessorRegisterTransferInstruction(Instruction):
    pass

class CoprocessorDataOperationInstruction(Instruction):
    pass

def InstructionFactory(addr,word):
    tag = (word>>26)&3
    handler = None
    if tag == 0:
        if (word&0xf0) != 0x90:
            handler = ALUInstruction
        elif (word&0xf00):
            handler = MultiplyInstruction
        else:
            handler = SwapInstruction
    elif tag == 1:
        handler = SingleDataTransferInstruction
    elif tag == 2:
        if word&0x02000000:
            handler = BranchInstruction
        else:
            handler = MultiDataTransferInstruction
    else:
        if (word&0x0f000000) == 0x0f000000:
            handler = SoftwareInterruptInstruction
        elif (word&0x02000000) == 0:
            handler = CoprocessorDataTransferInstruction
        elif word&0x10:
            handler = CoprocessorRegisterTransferInstruction
        else:
            handler = CoprocessorDataOperationInstruction
    return handler(addr,word)

def Disassemble(cpu,breakpoints,start,end):
    if start&3:
        raise ValueError
    for addr in xrange(start,end,4):
        if addr in breakpoints:
            word = breakpoints[addr]
        else:
            word = cpu.memw[addr]
        yield InstructionFactory(addr,word)
