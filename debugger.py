import curses
import disassemble
import time
import armv2

class WindowControl:
    SAME    = 1
    RESUME  = 2
    NEXT    = 3
    RESTART = 4
    EXIT    = 5

class View(object):
    def __init__(self,h,w,y,x):
        self.width  = w
        self.height = h
        self.startx = x
        self.starty = y
        self.window = curses.newwin(self.height,self.width,self.starty,self.startx)
        self.window.keypad(1)

    def Centre(self,pos):
        pass

    def Select(self,pos):
        pass

    def TakeInput(self):
        return WindowControl.SAME

class Debug(View):
    label_width = 14
    def __init__(self,debugger,h,w,y,x):
        super(Debug,self).__init__(h,w,y,x)
        self.selected = 0
        self.debugger = debugger

    def Centre(self,pos = None):
        if pos == None:
            pos = self.selected
        #Set our pos such that the given pos is as close to the centre as possible
        
        correct = None
        self.disassembly = []
        start = max(pos-((self.height-2)/2)*4,0)
        end = min(pos + ((self.height-2)/2)*4,len(self.debugger.machine.mem))
        #print '%x - %x - %x' % (start,pos,end)
        dis = []
        for instruction in disassemble.Disassemble(self.debugger.machine,self.debugger.breakpoints,start,start+(self.height-2)*4):
            arrow = '==>' if instruction.addr == self.debugger.machine.pc else ''
            bpt   = '*' if instruction.addr in self.debugger.breakpoints else ' '
            dis.append( (instruction.addr,'%3s%s%07x %08x : %s' % (arrow,bpt,instruction.addr,instruction.word,instruction.ToString())))
                
        self.disassembly = dis

    def Select(self,pos):
        self.selected = pos

    def TakeInput(self):
        ch = self.window.getch()
        #print ch,curses.KEY_DOWN
        if ch == curses.KEY_DOWN:
            try:
                self.selected = self.disassembly[self.selected_pos+1][0]
                self.Centre(self.selected)
            except IndexError:
                pass
        elif ch == curses.KEY_UP:
            if self.selected_pos > 0:
                self.selected = self.disassembly[self.selected_pos-1][0]
                self.Centre(self.selected)
        elif ch == curses.KEY_NPAGE:
            #We can't jump to any arbitrary point because we don't know the instruction boundaries
            #instead jump to the end of the screen twice, which should push us down by a whole page
            for i in xrange(2):
                p = self.disassembly[-1][0]
                self.Centre(p)

            self.Select(p)
        elif ch == curses.KEY_PPAGE:
            for i in xrange(2):
                p = self.disassembly[0][0]
                self.Centre(p)

            self.Select(p)
        elif ch == ord('\t'):
            return WindowControl.NEXT
        elif ch == ord(' '):
            if self.selected in self.debugger.breakpoints:
                self.debugger.RemoveBreakpoint(self.selected)
            else:
                self.debugger.AddBreakpoint(self.selected)
            self.Centre(self.selected)
        elif ch == ord('c'):
            self.debugger.Continue()
            return WindowControl.RESUME
        elif ch == ord('s'):
            self.debugger.Step()
            return WindowControl.RESUME
        elif ch == ord('r'):
            #self.debugger.machine.Reset()
            return WindowControl.RESUME
        elif ch == ord('q'):
            return WindowControl.EXIT
        return WindowControl.SAME


    def Draw(self,draw_border = False):
        self.window.clear()
        if draw_border:
            self.window.border()
        self.selected_pos = None
        for i,(pos,line) in enumerate(self.disassembly):
            if pos == self.selected:
                self.selected_pos = i
                self.window.addstr(i+1,1,line,curses.A_REVERSE)
            else:
                #print i,line
                self.window.addstr(i+1,1,line)
        self.window.refresh()

class State(View):
    reglist = [('r%d' % i) for i in xrange(12)] + ['fp','sp','lr','pc']
    mode_names = ['USR','FIQ','IRQ','SUP']
    def __init__(self,debugger,h,w,y,x):
        super(State,self).__init__(h,w,y,x)
        self.debugger = debugger

    def Draw(self,draw_border = False):
        self.window.clear()
        if draw_border:
            self.window.border()
        for i in xrange(16):
            regname = self.reglist[i]
            value = self.debugger.machine.regs[i]
            self.window.addstr(i+1,1,'%3s : %08x' % (regname,value))
        self.window.addstr(1,18,'Mode : %s' % self.mode_names[self.debugger.machine.mode])
        self.window.addstr(2,18,'  pc : %08x' % self.debugger.machine.pc)
        self.window.refresh()

class Help(View):
    def Draw(self,draw_border = False):
        self.window.clear()
        if draw_border:
            self.window.border()
        for i,(key,action) in enumerate( (('c','continue'),
                                          ('q','quit'),
                                          ('s','step'),
                                          ('space','set breakpoint'),
                                          ('tab','switch window')) ):
            self.window.addstr(i+1,1,'%5s - %s' % (key,action))
        self.window.refresh()

class Memdump(View):
    display_width = 16
    key_time = 1
    max = 0x4000000
    def __init__(self,debugger,h,w,y,x):
        super(Memdump,self).__init__(h,w,y,x)
        self.debugger = debugger
        self.pos      = 0
        self.selected = 0
        self.lastkey  = 0
        self.keypos   = 0
        self.masks = (0x3fffff0,0x3ffff00,0x3fff000,0x3ff0000,0x3f00000,0x3000000,0)
        self.newnum   = 0

    def Draw(self,draw_border = False):
        self.window.clear()
        if draw_border:
            self.window.border()
        for i in xrange(self.height-2):
            addr = self.pos + i*self.display_width
            data = self.debugger.machine.mem[addr:addr+self.display_width]
            if len(data) < self.display_width:
                data += '??'*(self.display_width-len(data))
            data_string = ' '.join((('%02x' % ord(data[i])) if i < len(data) else '??') for i in xrange(self.display_width))
            line = '%07x : %s' % (addr,data_string)
            if addr == self.selected:
                self.window.addstr(i+1,1,line,curses.A_REVERSE)
            else:
                self.window.addstr(i+1,1,line)
        
        type_string = ('%07x' % self.pos)[:self.keypos]
        extra = 7 - len(type_string)
        if extra > 0:
            type_string += '.'*extra
        self.window.addstr(1,60,type_string)
        self.window.refresh()

    def SetSelected(self,pos):
        if pos > self.max:
            pos = self.max
        if pos < 0:
            pos = 0
        self.selected = pos
        if ((self.selected - self.pos)/self.display_width) >= (self.height - 2):
            self.pos = self.selected - (self.height-3)*self.display_width
        elif self.selected < self.pos:
            self.pos = self.selected

    def TakeInput(self):
        ch = self.window.getch()
        if ch == curses.KEY_DOWN:
            self.SetSelected(self.selected + self.display_width)
        elif ch == curses.KEY_NPAGE:
            self.SetSelected(self.selected + self.display_width*(self.height-2))
        elif ch == curses.KEY_PPAGE:
            self.SetSelected(self.selected - self.display_width*(self.height-2))
        elif ch == curses.KEY_UP:
            self.SetSelected(self.selected - self.display_width)
        elif ch == ord('q'):
            return WindowControl.EXIT
        elif ch in [ord(c) for c in '0123456789abcdef']:
            newnum = int(chr(ch),16)
            self.keypos %= 7
            now = time.time()
            if now - self.lastkey > self.key_time:
                self.keypos = 0
                self.newnum = 0
            self.newnum <<= 4
            self.newnum |= newnum
            self.newnum &= 0x3ffffff
            self.pos &= self.masks[self.keypos]
            self.pos |= self.newnum
            self.keypos += 1
            self.lastkey = now
            self.selected = self.pos
            
        elif ch == ord('\t'):
            return WindowControl.NEXT
        return WindowControl.SAME
    

class Debugger(object):
    BKPT = 0xef000000 | armv2.SWI_BREAKPOINT
    FRAME_CYCLES = 66666
    def __init__(self,machine,stdscr):
        self.machine          = machine
        self.breakpoints      = {}
        self.selected         = 0
        self.stdscr           = stdscr
        #self.labels           = Labels(labels)

        self.h,self.w       = self.stdscr.getmaxyx()
        self.code_window    = Debug(self,self.h,self.w/2,0,0)
        self.state_window   = State(self,self.h/2,self.w/4,0,self.w/2)
        self.help_window    = Help(self.h/2,self.w/4,0,3*(self.w/4))
        self.memdump_window = Memdump(self,self.h/2,self.w/2,self.h/2,self.w/2)
        self.window_choices = [self.code_window,self.memdump_window]
        self.current_view   = self.code_window
        self.current_view.Select(self.machine.pc)
        self.current_view.Centre(self.machine.pc)
        self.num_to_step    = 0
        self.stopped        = True
        self.help_window.Draw()

    def AddBreakpoint(self,addr):
        if addr&3:
            raise ValueError()
        if addr in self.breakpoints:
            return
        addr_word = addr
        self.breakpoints[addr]   = self.machine.memw[addr_word]
        self.machine.memw[addr_word] = self.BKPT
    
    def RemoveBreakpoint(self,addr):
        self.machine.memw[addr] = self.breakpoints[addr]
        del self.breakpoints[addr]

    def StepNumInternal(self,num):
        #If we're at a breakpoint and we've been asked to continue, we step it once and then replace the breakpoint
        if num == 0:
            return None
        self.num_to_step -= num
        #print 'stepping',self.machine.pc,num, self.machine.pc in self.breakpoints
        if self.machine.pc in self.breakpoints:
            old_pos = self.machine.pc
            self.machine.memw[self.machine.pc] = self.breakpoints[self.machine.pc]
            self.machine.Step(1)
            self.machine.memw[old_pos] = self.BKPT
            if num > 0:
                num -= 1
        return self.machine.Step(num)
        

    def Step(self):
        return self.StepNumInternal(1)

    def Continue(self):
        result = None
        self.stopped = False
        try:
            if armv2.Status.Breakpoint == self.StepNumInternal(self.num_to_step):
                self.stopped = True
        except KeyboardInterrupt:
            armv2.DebugLog('kbd int 3')
            try:
                self.machine.cv.release()
            except RuntimeError:
                pass
            self.stopped = True
        
    def StepNum(self,num):
        self.num_to_step = num
        if not self.stopped:
            return self.Continue()
        #disassembly = disassemble.Disassemble(cpu.mem)
        #We're stopped, so display and wait for a keypress
        armv2.DebugLog('stopped, looking for stuff')
        try:
            for window in self.state_window,self.memdump_window,self.code_window:
                armv2.DebugLog(str(window))
                window.Draw(self.current_view is window)
                armv2.DebugLog(str(window) + ' 1')

            armv2.DebugLog('Taking Input')
            result = self.current_view.TakeInput()
            armv2.DebugLog('Got result %d' % result)
            if result == WindowControl.RESUME:
                self.current_view.Select(self.machine.pc)
                self.current_view.Centre(self.machine.pc)
            elif result == WindowControl.RESTART:
                return False
            elif result == WindowControl.NEXT:
                pos = self.window_choices.index(self.current_view)
                pos = (pos + 1)%len(self.window_choices)
                self.current_view = self.window_choices[pos]
            elif result == WindowControl.EXIT:
                raise SystemExit
        except KeyboardInterrupt:
            armv2.DebugLog('kbt int 2')
            self.stopped = True

    def Stop(self):
        armv2.DebugLog("Stopped called")
        self.stopped = True
