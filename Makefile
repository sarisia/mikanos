.PHONY: all
all: kernel

kernel: kernel.elf
kernel.elf: src/kernel/main.cpp
	clang++ ${CPPFLAGS} \
		-O2 -Wall -g --target=x86_64-elf \
		-ffreestanding \
		-mno-red-zone \
		-fno-exceptions \
		-fno-rtti \
		-std=c++17 \
		-c src/kernel/main.cpp
	ld.lld ${LDFLAGS} --entry KernelMain -z norelro --image-base=0x100000 --static \
		-o kernel.elf main.o

Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi: src/MikanLoaderPkg/Main.c
	build

.PHONY: run
run: Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi kernel.elf
	${HOME}/osbook/devenv/run_qemu.sh Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi kernel.elf

.PHONY: clean
clean:
	rm -f main.o kernel.elf
