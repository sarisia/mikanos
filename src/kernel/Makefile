TARGET=kernel.elf
OBJS= \
	main.o \
	graphics.o \
	font.o \
	hankaku.o \
	newlib_support.o \
	libcxx_support.o \
	console.o \
	pci.o \
	asmfunc.o \
	logger.o \
	mouse.o \
	interrupt.o \
	segment.o \
	paging.o \
	memory_manager.o \
	window.o \
	layer.o \
	timer.o \
	frame_buffer.o \
	acpi.o \
	keyboard.o \
	task.o \
	terminal.o \
	fat.o \
	usb/memory.o \
	usb/device.o \
	usb/classdriver/base.o \
	usb/classdriver/hid.o \
	usb/classdriver/keyboard.o \
	usb/classdriver/mouse.o \
	usb/xhci/ring.o \
	usb/xhci/trb.o \
	usb/xhci/xhci.o \
	usb/xhci/port.o \
	usb/xhci/device.o \
	usb/xhci/devmgr.o \
	usb/xhci/registers.o

CPPFLAGS+=-I../Include -I.

CFLAGS+= \
	-O2 -Wall -g --target=x86_64-elf \
	-ffreestanding \
	-mno-red-zone
CXXFLAGS+= \
	-O2 -Wall -g --target=x86_64-elf \
	-ffreestanding \
	-mno-red-zone \
	-fno-exceptions \
	-fno-rtti \
	-std=c++17

LDFLAGS+= --entry KernelMain -z norelro --image-base=0x100000 --static

.PHONY: all
all: ${TARGET}


${TARGET}: ${OBJS}
	ld.lld ${LDFLAGS} -o $@ $^ -lc -lc++ -lc++abi


kernel.lst: main.cpp
	clang++ ${CPPFLAGS} ${CXXFLAGS} -S -o kernel.lst main.cpp
	

%.o: %.cpp
	clang++ ${CPPFLAGS} ${CXXFLAGS} -o $@ -c $<

%.o: %.c
	clang ${CPPFLAGS} ${CFLAGS} -o $@ -c $<

%.o: %.asm
	nasm -f elf64 -o $@ $<


hankaku.bin: hankaku.txt
	../../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@


.PHONY: clean
clean:
	rm -rf *.o *.elf *.bin
	rm -rf usb/*.o usb/xhci/*.o usb/classdriver/*.o
