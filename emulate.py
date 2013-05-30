import armv2

cpu = armv2.Armv2(size = 2**16,
                  filename = 'boot.rom')

cpu.Step(10)
print cpu
