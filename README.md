# 8080 Emulator

*Reference guide can be found [here](http://www.emulator101.com/welcome.html)

*Also check out the intel 8080 manual in the reoi*

### Disassembler
To take the raw code and turn it into its assembly equivalent

### The CPU emulation itself

#### Instructions
The CPU executes instructions one at a time. Recreating the environment (registers, flags, memories) and applying the function of each instruction would essentially create an emulation of the CPU. It is not important how the actual hardware executes the code (it's a lot of electronics, electrical engineering and brain juice). All you're dong is recreating the effect of the instruction.
For example, adding 2 and the content of the accumulator would physically go though a couple of cycles that travel from the CPU to the ALU back to the register file, etc. It is not important to recreate the interaction at such a deep level. All you need to do is using whatever language you choose (C here) to perform the effect of the instruction, so basically:

```c
state->a += 2; //adding 2 to the acc
SetFlags(state->a);
cycles += 1;
// etc
```

which would virtually do the same as the actual microprocessor: add the content of the accumulator, set the flags, and whatever else is needed. The subsequent instructions could reference to any of the things we changed, and perform whatever they need to do in turn

#### Program counter
The program counter serves as the pointer to the instruction to execute. After the program is loaded into the memory, the PC "tells" the CPU what to perform. Essentially, it's simply moving from position *`n`* to position *`n+1`*. It is possible to jump to a specific instruction at a specific memory address which would set the PC to the memory index of the instruction to execute. This enables us to perform conditional operations, jump at specific address if certain conditions are met, or loop through a certain portion until the required condition is true. All of this, just by setting the PC to the right value.

`// add more later`

### The Space Invaders Hardware

The CPU alone is not enough to play the game. The CPU is just the 'brain' of the entire hardware.
Space Invaders in an arcade game, so we need to emulate the arcade's hardware as well

The very first version should be single player, with no sound for now. The focus is on getting the screen display right and the controls for one player right. From there the project can be considered complete. I will consider everything else as polishing the finished product.

Why? Because my focus is on the implementation of the CPU (can be reused later) and make the simplest implementation of the space invaders hardware.

For that you'll need

#### Buttons and ports

`// add more later`
#### Interrupts

`// add more later`
#### Special shift registers

`// add more later`
#### Video rendering unit

`// add more later`