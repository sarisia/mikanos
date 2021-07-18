TARGET=rpn

CPPFLAGS += -I.
CFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding \
	-fno-exceptions -fno-rtti -std=c++17
LDFLAGS += --entry main -z norelro --image-base 0 --static

.PHONY: all
all: ${TARGET}

rpn: rpn.o
	ld.lld $(LDFLAGS) -o $@ $< -lc -lc++ -lc++abi

%.o: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f ${TARGET} *.o
