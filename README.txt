To compile the simulator, enter the following command:

gcc -o spimcore spimcore.c project.c

Then, to run files through the simulator (with extension .asc), enter the following:

spimcore <inputfilename>.asc

To compile the assembler, enter the following command:

gcc -o assembler assembler.c

Then, to use the compiler to compile MIPS ASM code (.asm), enter the following:

assembler <inputfilename>.asm <outputfilename>.asc

An example .asm file (asm_test.asm) and its output (asm_test.asc) have been uploaded in this directory.