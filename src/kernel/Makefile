TARGET=kernel.elf
OBJS=main.o

CPPFLAGS+=-I../Include
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
	ld.lld ${LDFLAGS} -o $@ $^

%.o: %.cpp
	clang++ ${CPPFLAGS} ${CXXFLAGS} -c $<

.PHONY: clean
clean:
	rm -rf *.o *.elf
