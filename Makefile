.PHONY: all
all: kernel boot apps

.PHONY: kernel src/kernel/kernel.elf
kernel: src/kernel/kernel.elf
src/kernel/kernel.elf:
	make -C src/kernel

.PHONY: boot Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi
boot: Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi
Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi: src/MikanLoaderPkg/Main.c
	build

.PHONY: apps
apps:
	make -C src/apps


mikanos.img: boot kernel apps
	MIKANOS_DIR=$(abspath src) ./tools/make_os_image.sh


.PHONY: run
run: mikanos.img
	${HOME}/osbook/devenv/run_image.sh $<

.PHONY: debug
debug: export QEMU_OPTS = -gdb tcp::12345 -S
debug: run
# debug: Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi src/kernel/kernel.elf
# 	${HOME}/osbook/devenv/run_qemu.sh $^


.PHONY: clean
clean:
	rm -f *.img
	make -C src/kernel clean
	make -C src/apps clean

.PHONY: cleanall
cleanall: clean
	build cleanall
