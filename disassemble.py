
def Disassemble(cpu,start,end):
    if start&3:
        raise ValueError
    for addr in xrange(start,end,4):
        yield addr,cpu.memw[addr][0],'unknown',''
