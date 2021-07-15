TARGET=halt

.PHONY: all
all: ${TARGET}

halt: halt.asm
	nasm -f bin -o $@ $<

.PHONY: clean
clean:
	rm -f ${TARGET}
