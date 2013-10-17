This is a CPU emulator written in C with python bindings. It emulates an imaginary CPU that is very similar to the ARMv2 used in the Acorn Archimedes, however it is not designed to emulate anything in particular so I have taken some liberties with the implementation.

In particular, I have created a hardware co-processor that can be used to enumerate and interact with hardware devices, somewhat based on the ideas in the DCPU16 created by Mojang software. As I am intending to write all software for this CPU myself, for inclusion in a game that I'm working on, I can do what I want, but if you're looking for an accurate ARMv2 emulator, this isn't it

Tom
