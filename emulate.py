import armv2
import binascii
import debugger
import sys
import hardware
import pygame
from pygame.locals import *
from optparse import OptionParser

pygame.init()

class StdOutWrapper:
    text = []
    def write(self,txt):
        self.text.append(txt)
        if len(self.text) > 500:
            self.text = self.text[:500]
    def get_text(self):
        return ''.join(self.text)

def main(stdscr):
    parser = OptionParser(usage="usage: %prog [options] filename",
                          version="%prog 1.0")

    (options, args) = parser.parse_args()
    pygame.display.set_caption('ARM emulator')
    pygame.mouse.set_visible(0)

    curses.use_default_colors()
    machine = hardware.Machine(cpu_size = 2**21, cpu_rom = 'boot.rom')
    try:
        machine.AddHardware(hardware.Keyboard())
        machine.AddHardware(hardware.LCDDisplay(),name='display')

        dbg = debugger.Debugger(machine,stdscr)
        background = pygame.Surface((200,200))
        background = background.convert()
        background.fill((0, 0, 0))
        machine.display.screen.blit(background, (0, 0))

        def frame_callback():
            print 'frame'

        dbg.Run(frame_callback)
    finally:
        machine.Delete()

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
        
