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

global GetCR3 ; uint64_t GetCR3()
GetCR3:
    mov rax, cr3
    ret

global SwitchContext ; void SwitchContext(void *next_ctx, void *current_ctx)
SwitchContext:
    ; save current context
    mov [rsi+0x40], rax
    mov [rsi+0x48], rbx
    mov [rsi+0x50], rcx
    mov [rsi+0x58], rdx
    mov [rsi+0x60], rdi
    mov [rsi+0x68], rsi

    lea rax, [rsp+8] ; load effective address
    mov [rsi+0x70], rax ; rsp
    mov [rsi+0x78], rbp

    mov [rsi+0x80], r8
    mov [rsi+0x88], r9
    mov [rsi+0x90], r10
    mov [rsi+0x98], r11
    mov [rsi+0xa0], r12
    mov [rsi+0xa8], r13
    mov [rsi+0xb0], r14
    mov [rsi+0xb8], r15

    mov rax, cr3
    mov [rsi+0x00], rax ; cr3
    mov rax, [rsp]
    mov [rsi+0x08], rax ; rip
    pushfq ; push rFLAGS Register onto the stack
    pop qword [rsi+0x10]

    mov ax, cs
    mov [rsi+0x20], rax ; cs
    mov bx, ss
    mov [rsi+0x28], rbx ; ss
    mov cx, fs
    mov [rsi+0x30], rcx ; fs
    mov dx, gs
    mov [rsi+0x38], rdx ; gs

    fxsave [rsi+0xc0] ; save float related

    ; iret
    push qword [rdi+0x28] ; ss
    push qword [rdi+0x70] ; rsp
    push qword [rdi+0x10] ; rflags
    push qword [rdi+0x20] ; cs
    push qword [rdi+0x08] ; rip

    ; restore context
    fxrstor [rdi+0xc0]

    mov rax, [rdi+0x00]
    mov cr3, rax
    mov rax, [rdi+0x30]
    mov fs, ax
    mov rax, [rdi+0x38]
    mov gs, ax

    mov rax, [rdi+0x40]
    mov rbx, [rdi+0x48]
    mov rcx, [rdi+0x50]
    mov rdx, [rdi+0x58]
    mov rsi, [rdi+0x68]
    mov rbp, [rdi+0x78]

    mov r8, [rdi+0x80]
    mov r9, [rdi+0x88]
    mov r10, [rdi+0x90]
    mov r11, [rdi+0x98]
    mov r12, [rdi+0xa0]
    mov r13, [rdi+0xa8]
    mov r14, [rdi+0xb0]
    mov r15, [rdi+0xb8]

    mov rdi, [rdi+0x60]

    o64 iret
