objs = start.o main.o
ledc.bin: $(objs)
	arm-linux-gnueabihf-ld -Ttext 0x87800000 $^ -o ledc.elf
	arm-linux-gnueabihf-objcopy -O binary -S ledc.elf $@
	arm-linux-gnueabihf-objdump -D -m arm ledc.elf > ldec.dis

%.o : %.c
	arm-linux-gnueabihf-gcc -Wall -nostdlib -c -o $@ $<
%.o : %.s
	arm-linux-gnueabihf-gcc -Wall -nostdlib -c -o $@ $<
%.o : %.S
	arm-linux-gnueabihf-gcc -Wall -nostdlib -c -o $@ $<
	
clean:
	rm -rf *.o ledc.bin ledc.elf ledc.dis
