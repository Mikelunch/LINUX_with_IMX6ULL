ARM     ?= arm-linux-gnueabihf
#修改工程名
TARGET ?= key
#修改输出文件路径
OUTPUT ?= output

LD             := $(ARM)-ld
GCC            := $(ARM)-gcc
OBJCOPY        := $(ARM)-objcopy
OBJDUMP        := $(ARM)-objdump

#添加头文件路径
INCUDIRS       := imx6u \
			   bsp/clk \
			   bsp/delay \
			   bsp/led \
			   bsp/beep \
			   bsp/key

#添加源文件路径
SRCDIRS        :=  proj \
			   bsp/clk \
			   bsp/delay \
			   bsp/led \
			   bsp/beep \
			   bsp/key

INCLUDE        :=  $(patsubst %, -I %,$(INCUDIRS))

SFILES         := $(foreach dir , $(SRCDIRS) , \
			   $(wildcard $(dir)/*.S))
sFILES         := $(foreach dir , $(SRCDIRS) , \
			   $(wildcard $(dir)/*.s))
CFILES         := $(foreach dir , $(SRCDIRS) , \
               $(wildcard $(dir)/*.c))
SFILENDIR      := $(notdir $(SFILES))
CFILENDIR      := $(notdir $(CFILES))
sFILENDIR      := $(notdir $(sFILES))

sOBJS          := $(patsubst %, obj/%, \
			   $(sFILENDIR:.s=.o))
COBJS          := $(patsubst %, obj/%, \
			   $(CFILENDIR:.c=.o))
OBJS           := $(sOBJS)$(COBJS)

VPATH          := $(SRCDIRS)

.PHONY:clean

$(TARGET).bin : $(OBJS)
	$(LD) -Timx6u.lds -o $(TARGET).elf $^
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis
	mv $(TARGET).bin ./$(OUTPUT)
	mv $(TARGET).elf ./$(OUTPUT)
	mv $(TARGET).dis ./$(OUTPUT)

$(sOBJS) : obj/%.o : %.s
	$(GCC) -Wall -nostdlib -c $(INCLUDE) -o $@ $<
$(COBJS) : obj/%.o : %.c
	$(GCC) -Wall -nostdlib -c $(INCLUDE) -o $@ $<

clean:
	rm -r ./obj/* ./output/*
	
download:
	./imxdownload ./$(OUTPUT)/$(TARGET).bin /dev/sdb

print:
	@echo CFILENDIR = $(CFILENDIR)

