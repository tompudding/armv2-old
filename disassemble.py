
registerNames = [('r%d' % i) for i in xrange(13)] + ['sp','lr','pc']
shiftTypes = ['LSL','LSR','ASR','ROR']

def OperandShift(self,bits,type_flag):
    rm = registerNames[bits&0xf]
    shift_type = shiftTypes[(bits>>5)&0x3]
    if type_flag:
        #shift amount is a register
        shift_val = None
        shift = registerNames[(bits>>8)&0xf]
    else:
        shift_val = (bits>>7)&0x1f
        shift = '#%d' % (shift_val)
    if shift_type == 'ROR' and type_flag == 0 and shift_val == 0:
        return [rm,'RRX']
    elif shift_val == 0:
        return [rm]
    else:
        #shift is nonzero
        return [rm,shift_type + ' ' + shift]

def RegisterList(bits):
    runs = []
    on = None
    for i in xrange(16):
        if on == None and (bits>>i)&1:
            on = i
        elif on != None and not ((bits>>i)&1):
            runs.append( (on,i) )
            on = None
    if on:
        runs.append( (on,15) )
    regs = []
    for start,end in runs:
        if end-start <= 2:
            for r in xrange(start,end):
                regs.append(registerNames[r])
        else:
            regs.append('%s - %s' % (registerNames[start],registerNames[end]))
    return ['{' + ','.join(regs) + '}']

class Instruction(object):
    conditions = ['EQ','NE','CS','CC',
                  'MI','PL','VS','VC',
                  'HI','LS','GE','LT',
                  'GT','LE','','NV']
    mneumonic = 'UNK'
    args = []
    def __init__(self,addr,word,cpu):
        self.addr = addr
        self.word = word
    def ToString(self):
        return '%5s%2s %s' % (self.mneumonic,self.conditions[(self.word>>28)&0xf],', '.join(self.args))

class ALUInstruction(Instruction):
    opcodes = ['AND','EOR','SUB','RSB',
               'ADD','ADC','SBC','RSC',
               'TST','TEQ','CMP','CMN',
               'ORR','MOV','BIC','MVN']
    MOV = 0xd
    MVN = 0xf
    ALU_TYPE_IMM = 0x02000000
    def __init__(self,addr,word,cpu):
        super(ALUInstruction,self).__init__(addr,word,cpu)
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
            op2 = OperandShift(addr,word&0xfff,word&0x10)
        if opcode in [self.MOV,self.MVN]:
            self.args = [rd] + op2
        else:
            self.args = [rd,op1] + op2

class MultiplyInstruction(Instruction):
    MUL_TYPE_MLA = 0x00200000
    def __init__(self,addr,word,cpu):
        super(MultiplyInstruction,self).__init__(addr,word,cpu)
        rm = word&0xf
        rn = (word>>12)&0xf
        rs = (word>> 8)&0xf
        rd = (word>>16)&0xf
        self.args = [registerNames[rd],registerNames[rm],registerNames[rs]]
        if word&self.MUL_TYPE_MLA:
            self.mneumonic = 'MLA'
            self.args.append(registerNames[rn])
        else:
            self.mneumonic = 'MUL'

class SwapInstruction(Instruction):
    LOAD_BYTE = 0x00400000
    def __init__(self,addr,word,cpu):
        super(SwapInstruction,self).__init__(addr,word,cpu)
        rm   = word&0xf;
        rd   = (word>>12)&0xf
        rn   = (word>>16)&0xf
        if word&self.LOAD_BYTE:
            self.mneumonic = 'SWPB'
        else:
            self.mneumonic = 'SWP'
        self.args = [registerNames[rd],registerNames[rm],'[%s]' % registerNames[rn]]

class SingleDataTransferInstruction(Instruction):
    SDT_REGISTER   = 0x02000000
    SDT_WRITE_BACK = 0x00200000
    SDT_PREINDEX   = 0x01000000
    SDT_LDR        = 0x00100000
    SDT_OFFSET_ADD = 0x00800000
    def __init__(self,addr,word,cpu):
        super(SingleDataTransferInstruction,self).__init__(addr,word,cpu)
        rd = (word>>12)&0xf;
        rn = (word>>16)&0xf;
        rd = registerNames[rd]
        if not word&self.SDT_REGISTER:
            offset = word&0xfff
            if not word&self.SDT_OFFSET_ADD:
                offset *= -1
            op2 = ['#0x%x' % offset]
        else:
            op2 = OperandShift(addr,word&0xfff,0)
            #FIXME, incorporate the negativeness
        if word&self.SDT_LDR:
            self.mneumonic = 'LDR'
        else:
            self.mneumonic = 'STR'
        if word&self.SDT_WRITE_BACK:
            self.mneumonic += '!'
        
        if rn == 0xf and not word&self.SDT_REGISTER:
            val = cpu.memw[addr+8+offset]
            self.args = [rd] + ['=0x%x' % val]
            return
            
        rn = registerNames[rn]
        op2.insert(0,rn)
        
        
        op2[0] = '[' + op2[0]
        if word&self.SDT_PREINDEX:
            op2[-1] = op2[-1] + ']'
        else:
            op2[0] = op2[0] + ']'
        self.args = [rd] + op2
        

class BranchInstruction(Instruction):
    def __init__(self,addr,word,cpu):
        super(BranchInstruction,self).__init__(addr,word,cpu)
        if (word>>24)&1:
            self.mneumonic = 'BL'
        else:
            self.mneumonic = 'B'
        offset = (word&0xffffff)<<2
        self.args = ['#0x%x' % (addr + offset + 8)]

class MultiDataTransferInstruction(Instruction):
    MDT_LDM        = 0x00100000
    MDT_WRITE_BACK = 0x00200000
    MDT_HAT        = 0x00400000
    MDT_OFFSET_ADD = 0x00800000
    MDT_PREINDEX   = 0x01000000
    def __init__(self,addr,word,cpu):
        super(MultiDataTransferInstruction,self).__init__(addr,word,cpu)
        if word&self.MDT_LDM:
            self.mneumonic = 'LDM'
        else:
            self.mneumonic = 'STM'
        if word&self.MDT_OFFSET_ADD:
            self.mneumonic += 'I'
        else:
            self.mneumonic += 'D'
        if word&self.MDT_PREINDEX:
            self.mneumonic += 'B'
        else:
            self.mneumonic += 'A'
        rn = registerNames[(word>>16)&0xf] + '!' if word&self.MDT_WRITE_BACK else ''
        self.args = [rn] + RegisterList(word&0xffff)
        

class SoftwareInterruptInstruction(Instruction):
    mneumonic = 'SWI'
    def __init__(self,addr,word,cpu):
        super(SoftwareInterruptInstruction,self).__init__(addr,word,cpu)
        self.args = ['#0x%x' % (word&0xffffff)]

class CoprocessorDataTransferInstruction(Instruction):
    pass

class CoprocessorRegisterTransferInstruction(Instruction):
    pass

class CoprocessorDataOperationInstruction(Instruction):
    pass

def InstructionFactory(addr,word,cpu):
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
    return handler(addr,word,cpu)

def Disassemble(cpu,breakpoints,start,end):
    if start&3:
        raise ValueError
    for addr in xrange(start,end,4):
        if addr in breakpoints:
            word = breakpoints[addr]
        else:
            word = cpu.memw[addr]
        yield InstructionFactory(addr,word,cpu)
