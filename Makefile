kernel: kernel.elf
kernel.elf: src/kernel/main.cpp
	clang++ -O2 -Wall -g --target=x86_64-elf \
		-ffreestanding \
		-mno-red-zone \
		-fno-exceptions \
		-fno-rtti \
		-std=c++17 \
		-c src/kernel/main.cpp
	ld.lld --entry KernelMain -z norelro --image-base=0x100000 --static \
		-o kernel.elf main.o

.PHONY: clean
clean:
	rm -f main.o kernel.elf
