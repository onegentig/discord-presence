; Assembly for Wine on x86_64 using MASM syntax

public wine_close
public wine_open
public wine_read
public wine_write
public wine_socket
public wine_connect
public wine_fcntl

.code

wine_close proc ; int wine_close(uint32_t fd);
    push rdi
    push rax

    mov [rsp+4], ecx
    mov edi, [rsp+4]

    mov rax, 3
    syscall

    mov [rsp], eax
    mov eax, [rsp]
    add rsp, 8
    pop rdi
    ret
wine_close endp

wine_open proc ; int wine_open(const char* path, int flags, int mode);
    push rsi
    push rdi

    sub rsp, 18h
    mov [rsp+14h], r8d
    mov [rsp+10h], edx
    mov [rsp+8], rcx

    mov rdi, [rsp+8]
    mov esi, [rsp+10h]
    mov edx, [rsp+14h]

    mov rax, 2
    syscall

    mov [rsp+4], eax
    mov eax, [rsp+4]
    add rsp, 18h

    pop rdi
    pop rsi
    ret
wine_open endp

wine_read proc ; ssize_t wine_read(uint32_t fd, void* buffer, size_t count);
    push rsi
    push rdi

    sub rsp, 20h
    mov [rsp+18h], r8
    mov [rsp+10h], rdx
    mov [rsp+8], ecx

    mov edi, [rsp+8]
    mov rsi, [rsp+10h]
    mov rdx, [rsp+18h]

    mov rax, 0
    syscall

    mov [rsp], rax
    mov rax, [rsp]
    add rsp, 20h

    pop rdi
    pop rsi
    ret
wine_read endp

wine_write proc ; ssize_t wine_write(uint32_t fd, const void* buf, size_t count);
    push rsi
    push rdi

    sub rsp, 20h
    mov [rsp+18h], r8
    mov [rsp+10h], rdx
    mov [rsp+8], ecx

    mov edi, [rsp+8]
    mov rsi, [rsp+10h]
    mov rdx, [rsp+18h]

    mov rax, 1
    syscall

    mov [rsp], rax
    mov rax, [rsp]
    add rsp, 20h

    pop rdi
    pop rsi
    ret
wine_write endp

wine_socket proc ; int wine_socket(int domain, int type, int protocol);
    push rsi
    push rdi

    sub rsp, 18h
    mov [rsp+14h], r8d
    mov [rsp+10h], edx
    mov [rsp+8], ecx

    mov edi, [rsp+8]
    mov esi, [rsp+10h]
    mov edx, [rsp+14h]

    mov rax, 29h
    syscall

    mov [rsp+4], eax
    mov eax, [rsp+4]
    add rsp, 18h

    pop rdi
    pop rsi
    ret
wine_socket endp

wine_connect proc ; int wine_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
    push rsi
    push rdi

    sub rsp, 18h
    mov [rsp+14h], r8d
    mov [rsp+8], rdx
    mov [rsp+4], ecx

    mov edi, [rsp+4]
    mov rsi, [rsp+8]
    mov edx, [rsp+14h]

    mov rax, 2Ah
    syscall

    mov [rsp], eax
    mov eax, [rsp]
    add rsp, 18h

    pop rdi
    pop rsi
    ret
wine_connect endp

wine_fcntl proc ; int wine_fcntl(int fd, int cmd, int arg);
    push rsi
    push rdi

    sub rsp, 10h
    mov [rsp+20h-14h], r8d
    mov [rsp+8], edx
    mov [rsp+4], ecx

    mov edi, [rsp+4]
    mov esi, [rsp+8]
    mov edx, [rsp+20h-14h]

    mov rax, 48h
    syscall

    mov [rsp], eax
    mov eax, [rsp]
    add rsp, 10h

    pop rdi
    pop rsi
    ret
wine_fcntl endp

END