bits 64
section .text

; Registers: RDI, RSI, RDX, RCX, R8, R9

global IoOut32 ; void IoOut32(uint16_t addr, uint32_t data)
IoOut32:
    mov dx, di ; dx = addr
    mov eax, esi ; eax = data
    out dx, eax
    ret

global IoIn32 ; uint32_t IoIn32(uint16_t addr)
IoIn32:
    mov dx, di ; dx = addr
    in eax, dx
    ret

global GetCS ; uint64_t GetCS(void)
GetCS:
    xor eax, eax
    mov ax, cs
    ret

global LoadIDT ; void LoadIDT(uint64_t limit, uint64_t offset)
LoadIDT:
    push rbp
    mov rbp, rsp ; rbp = rsp, use rbp as base memory address
    sub rsp, 10
    mov [rsp], di ; limit
    mov [rsp+2], rsi ; offset
    lidt [rsp] ; Load Interrupt Descriptor Table Register
    mov rsp, rbp
    pop rbp
    ret
