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

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
    mov rsp, kernel_main_stack+1024*1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin

global LoadGDT ; void LoadGDT(uint16_t limit, uint64_t offset)
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di ; limit
    mov [rsp+2], rsi ; offset
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global SetDSAll ; void SetDSAll(uint16_t value)
SetDSAll:
    mov ds, di ; not used
    mov es, di ; not used
    mov fs, di ; not used
    mov gs, di ; not used
    ret

global SetCSSS ; void SetCSSS(uint16_t cs, uint16_t ss)
SetCSSS:
    push rbp
    mov rbp, rsp
    mov ss, si ; ss
    mov rax, .next
    push rdi ; cs
    push rax ; rip, return addr
    o64 retf ; far jump
.next:
    mov rsp, rbp
    pop rbp
    ret

global SetCR3 ; void SetCR3(uint64_t value)
SetCR3:
    mov cr3, rdi
    ret
