import armv2

cpu = armv2.Armv2(size = 2**16,
                  filename = 'boot.rom')

cpu.Step(1)
print cpu.regs.pc
cpu.regs.pc = 3
cpu.Step(1)
print cpu.regs.pc
