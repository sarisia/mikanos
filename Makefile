.PHONY: all
all: kernel

.PHONY: kernel src/kernel/kernel.elf
kernel: src/kernel/kernel.elf
src/kernel/kernel.elf:
	make -C src/kernel

Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi:
	build

.PHONY: run
run: Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi src/kernel/kernel.elf
	${HOME}/osbook/devenv/run_qemu.sh $^

.PHONY: clean
clean:
	build cleanall
	make -C src/kernel clean
