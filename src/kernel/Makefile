TARGET=kernel.elf
OBJS= \
	main.o \
	graphics.o \
	font.o \
	hankaku.o \
	newlib_support.o \
	console.o

CPPFLAGS+=-I../Include

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
	ld.lld ${LDFLAGS} -o $@ $^ -lc


kernel.lst: main.cpp
	clang++ ${CPPFLAGS} ${CXXFLAGS} -S -o kernel.lst main.cpp
	

%.o: %.cpp
	clang++ ${CPPFLAGS} ${CXXFLAGS} -c $<

%.o: %.c
	clang ${CPPFLAGS} ${CFLAGS} -c $<


hankaku.bin: hankaku.txt
	../../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@


.PHONY: clean
clean:
	rm -rf *.o *.elf *.bin
