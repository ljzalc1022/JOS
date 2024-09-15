# JOS

This lab from [MIT 6.828 Operating System Engineering](https://pdos.csail.mit.edu/6.828/2018/tools.html) implements the essential parts of a operating system step by steps.

This OS can manage system with `x86` CPUs and has multicore supports. One can try this out by using [QEMU](https://www.qemu.org/) (a x86 platform and CPU emulator)

Conceputally, it can be used to manage bare machine like any other OSes but this is not ever tested.

## Reports

The code implemented by the author is explained in the reports of each steps of the lab. see `reports/`

1. Booting a PC
2. Memory Management
3. User Environments
4. Preemptive Multitasking
5. Filesystem, Spawn and Shell

## Test

To test this OS, try

```bash
make qemu
make qemu-nox # run without GUI
```

> This project needs a cross platform GNU-toolchain which supports i386-elf format. Check whether your toolchain satisifies this by `objdump -i | grep 'elf32-i386'`

