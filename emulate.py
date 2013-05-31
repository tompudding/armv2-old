import armv2
import binascii

cpu = armv2.Armv2(size = 2**16,
                  filename = 'boot.rom')

cpu.Step(1)
print cpu.regs.pc
cpu.regs.pc = 3
cpu.Step(1)
print cpu.regs.pc
cpu.regs[:4] = [1,2,3,4]
print cpu.regs

backup = cpu.mem[:9]

cpu.mem[:9] = 'abcdefghi'
print binascii.hexlify(cpu.mem[:12])
cpu.mem[:9] = backup
print binascii.hexlify(cpu.mem[:12])
print cpu.memw[:3]
