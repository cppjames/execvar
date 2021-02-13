# execvar

This is a C program that counts the number of times it has been executed by modifying its own
executable.

Note: this program is linux/elf only.

### Documentation:

`dladdr1` is used to find the program's base address in memory and reverse the PIE or ASLR address
space randomization. After computing the address of the data in ELF virtual address space, the
binary's ELF section headers are scanned to find the .data section containing the program data we're
updating. We then rewrite the program executable file.
