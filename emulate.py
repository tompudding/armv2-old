import armv2
import binascii
import debugger
import sys

class StdOutWrapper:
    text = []
    def write(self,txt):
        self.text.append(txt)
        if len(self.text) > 500:
            self.text = self.text[:500]
    def get_text(self):
        return ''.join(self.text)

def main(stdscr):
    curses.use_default_colors()
    cpu = armv2.Armv2(size = 2**21,
                      filename = 'boot.rom')

    dbg = debugger.Debugger(cpu,stdscr)
    dbg.Run()

if __name__ == '__main__':
    import curses
    mystdout = StdOutWrapper()
    sys.stdout = mystdout
    sys.stderr = mystdout
    try:
        curses.wrapper(main)
    finally:
        sys.stdout = sys.__stdout__
        sys.stderr = sys.__stderr__
        sys.stdout.write(mystdout.get_text())
        
