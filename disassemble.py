
def Disassemble(cpu,breakpoints,start,end):
    if start&3:
        raise ValueError
    for addr in xrange(start,end,4):
        if addr in breakpoints:
            word = breakpoints[addr]
        else:
            word = cpu.memw[addr]
        yield addr,word,'unknown',''
