talget := start.o main.o
ld := arm-linux-gnueabihf-ld
gcc := arm-linux-gnueabihf-gcc
objcopy := arm-linux-gnueabihf-objcopy
objdump := arm-linux-gnueabihf-objdump

ledc.bin : $(talget)
	$(ld) -Timux6u.lds -o ledc.elf &^
	$(objcopy) -O binary -S ledc.elf &@
	$(objdump) -D -m arm ledc.elf > ledc.dis

%.o : %.c
	$(gcc) -Wall -nostdlib -c -o $@ $^
%.o : %.s
	$(gcc) -Wall -nostdlib -c -o $@ $^
%.o : %.S
	$(gcc) -Wall -nostdlib -c -o $@ $^

clean:
	rm -rf *.o *.elf *.dis *.bin