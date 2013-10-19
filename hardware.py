import armv2
import pygame
import threading

class Keyboard(armv2.Device):
    id = 0x41414141
    def __init__(self):
        super(Keyboard,self).__init__()
        armv2.DebugLog('keyboard keyboard keyboard!\n')

    def readCallback(self,addr,value):
        armv2.DebugLog('keyboard reader %x %x\n' % (addr,value))
        return 0;

    def writeCallback(self,addr,value):
        armv2.DebugLog('keyboard writer %x %x\n' % (addr,value))

class LCDDisplay(armv2.Device):
    id = 0x41414142
    char_size = (40,48)
    font = {' '  : 0x0000, '!'  : 0x0101, '"'  : 0x0008, '#'  : 0x34e8,
            '$'  : 0x34cb, '%'  : 0x1212, '&'  : 0x2955, '\'' : 0x0804,
            '('  : 0x2103, ')'  : 0x3021, '*'  : 0x0e1c, '+'  : 0x04c8,
            ','  : 0x0200, '-'  : 0x00c0, '.'  : 0x0400, '/'  : 0x0210,
            '0'  : 0x0000, '1'  : 0x0000, '2'  : 0x0000, '3'  : 0x0000,
            '4'  : 0x0000, '5'  : 0x0000, '6'  : 0x0000, '7'  : 0x0000,
            '8'  : 0x0000, '9'  : 0x0000, ':'  : 0x0000, ';'  : 0x0000,
            '<'  : 0x0000, '='  : 0x0000, '>'  : 0x0000, '?'  : 0x0000,
            '@'  : 0x0000, 'A'  : 0x0000, 'B'  : 0x0000, 'C'  : 0x0000,
            'D'  : 0x0000, 'E'  : 0x0000, 'F'  : 0x0000, 'G'  : 0x0000,
            'H'  : 0x0000, 'I'  : 0x0000, 'J'  : 0x0000, 'K'  : 0x0000,
            'L'  : 0x0000, 'M'  : 0x0000, 'N'  : 0x0000, 'O'  : 0x0000,
            'P'  : 0x0000, 'Q'  : 0x0000, 'R'  : 0x0000, 'S'  : 0x0000,
            'T'  : 0x0000, 'U'  : 0x0000, 'V'  : 0x0000, 'W'  : 0x0000,
            'X'  : 0x0000, 'Y'  : 0x0000, 'Z'  : 0x0000, '['  : 0x0000,
            '\\' : 0x0000, ']'  : 0x0000, '^'  : 0x0000, '_'  : 0x0000,
            '`'  : 0x0000, 'a'  : 0x0000, 'b'  : 0x0000, 'c'  : 0x0000,
            'd'  : 0x0000, 'e'  : 0x0000, 'f'  : 0x0000, 'g'  : 0x0000,
            'h'  : 0x0000, 'i'  : 0x0000, 'j'  : 0x0000, 'k'  : 0x0000,
            'l'  : 0x0000, 'm'  : 0x0000, 'n'  : 0x0000, 'o'  : 0x0000,
            'p'  : 0x0000, 'q'  : 0x0000, 'r'  : 0x0000, 's'  : 0x0000,
            't'  : 0x0000, 'u'  : 0x0000, 'v'  : 0x0000, 'w'  : 0x0000,
            'x'  : 0x0000, 'y'  : 0x0000, 'z'  : 0x0000, '{'  : 0x0000,
            '|'  : 0x0000, '}'  : 0x0000, '~'  : 0x0000, '' : 0x0000}

    def __init__(self):
        super(LCDDisplay,self).__init__()
        self.size_in_chars = (14,1)
        self.size = [self.size_in_chars[i]*self.char_size[i] for i in (0,1)]
        self.screen = pygame.display.set_mode(self.size)
        armv2.DebugLog('LCD Init!\n')

    def readCallback(self,addr,value):
        armv2.DebugLog('keyboard reader %x %x\n' % (addr,value))
        return 0;

    def writeCallback(self,addr,value):
        armv2.DebugLog('keyboard writer %x %x\n' % (addr,value))

class MemPassthrough(object):
    def __init__(self,cv,accessor):
        self.cv = cv
        self.accessor = accessor

    def __getitem__(self,index):
        with self.cv:
            return self.accessor.__getitem__(index)

    def __setitem__(self,index,values):
        with self.cv:
            return self.accessor.__setitem__(index,values)

    def __len__(self):
        with self.cv:
            return self.accessor.__len__()

class Machine:
    def __init__(self,cpu_size,cpu_rom):
        self.cpu          = armv2.Armv2(size = cpu_size,filename = cpu_rom)
        self.hardware     = []
        self.running      = True
        self.steps_to_run = 0
        self.cv           = threading.Condition()
        self.mem          = MemPassthrough(self.cv,self.cpu.mem)
        self.memw         = MemPassthrough(self.cv,self.cpu.memw)
        self.thread       = threading.Thread(target = self.threadMain)
        self.thread.start()

    @property
    def regs(self):
        with self.cv:
            return self.cpu.regs

    @regs.setter
    def regs(self,value):
        with self.cv:
            self.cpu.regs = value

    @property
    def mode(self):
        armv2.DebugLog('About to lock in mode')
        with self.cv:
            return self.cpu.mode

    @property
    def pc(self):
        armv2.DebugLog('About to lock in pc')
        with self.cv:
            return self.cpu.pc

    def threadMain(self):
        with self.cv:
            while self.running:
                armv2.DebugLog('main thread loop')
                while self.running and self.steps_to_run == 0:
                    armv2.DebugLog('about to wait')
                    self.cv.wait()
                if not self.running:
                    break
                armv2.DebugLog('Cpu thread running with %d ticks' % self.steps_to_run)
                self.cpu.Step(self.steps_to_run)
                self.steps_to_run = 0

    def Step(self,num):
        armv2.DebugLog('step lock %d' % num)
        with self.cv:
            self.steps_to_run = num
            self.cv.notify()

    def AddHardware(self,device,name = None):
        armv2.DebugLog('add hardware lock')
        with self.cv:
            self.cpu.AddHardware(device)
        self.hardware.append(device)
        if name != None:
            setattr(self,name,device)

    def Delete(self):
        armv2.DebugLog('Killing machine')
        with self.cv:
            self.running = False
            self.cv.notify()
        self.thread.join()
        armv2.DebugLog('Killed')

   
