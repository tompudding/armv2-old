import os
import sys

def load(filename):
    with open(filename,'rb') as f:
        return f.read()

boot,data = (load(filename) for filename in sys.argv[1:3])

assert len(boot) < 0x8000
boot = boot + '\x00'*(0x8000 - len(boot))
with open(sys.argv[3],'wb') as f:
    f.write(boot)
    f.write(data[0x8000:])
