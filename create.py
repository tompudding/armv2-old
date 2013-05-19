import os
import sys
import struct

def load(filename):
    with open(filename,'rb') as f:
        return f.read()

boot,data = (load(filename) for filename in sys.argv[1:3])

#Get rid of any bx lrs from the gcc stdlib

data = data.replace(struct.pack('<I',0xe12fff1e),struct.pack('<I',0xe1a0f00e))

assert len(boot) < 0x8000
boot = boot + '\x00'*(0x8000 - len(boot))
with open(sys.argv[3],'wb') as f:
    f.write(boot)
    f.write(data[0x8000:])
