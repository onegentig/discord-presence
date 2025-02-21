#pragma once

// Inline assembly for Wine on x86 taken from:
// https://github.com/0e4ef622/wine-discord-ipc-bridge/blob/master/main.c

inline __declspec(naked) int wine_close(uint32_t fd) {
    __asm__ (
        "push ebx\n"

        "mov eax, 0x06\n"
        "mov ebx, [esp + 4 + 4]\n"
        "int 0x80\n"

        "pop ebx\n"
        "ret\n"
    );
}

inline __declspec(naked) int wine_open(const char* path, int flags, int mode) {
    __asm__ (
        "push ebx\n"

        "mov eax, 0x05\n"
        "mov ebx, [esp + 4 + 4]\n"
        "mov ecx, [esp + 4 + 8]\n"
        "mov edx, [esp + 4 + 12]\n"
        "int 0x80\n"

        "pop ebx\n"
        "ret\n"
    );
}

inline __declspec(naked) int wine_read(uint32_t fd, void* buffer, uint32_t count) {
    __asm__ (
        "push ebx\n"

        "mov eax, 0x03\n"
        "mov ebx, [esp + 4 + 4]\n"
        "mov ecx, [esp + 4 + 8]\n"
        "mov edx, [esp + 4 + 12]\n"
        "int 0x80\n"

        "pop ebx\n"
        "ret\n"
    );
}

inline __declspec(naked) int wine_write(uint32_t fd, const char* buf, uint32_t count) {
    __asm__ (
        "push ebx\n"

        "mov eax, 0x04\n"
        "mov ebx, [esp + 4 + 4]\n"
        "mov ecx, [esp + 4 + 8]\n"
        "mov edx, [esp + 4 + 12]\n"
        "int 0x80\n"

        "pop ebx\n"
        "ret"
    );
}

inline __declspec(naked) int wine_socketcall(int call, void* args) {
    __asm__ (
        "push ebx\n"

        "mov eax, 0x66\n"
        "mov ebx, [esp + 4 + 4]\n"
        "mov ecx, [esp + 4 + 8]\n"
        "int 0x80\n"

        "pop ebx\n"
        "ret\n"
    );
}

inline int wine_socket(int domain, int type, int protocol) {
    void* args[3];
    args[0] = reinterpret_cast<void*>(domain);
    args[1] = reinterpret_cast<void*>(type);
    args[2] = reinterpret_cast<void*>(protocol);
    return wine_socketcall(1, args);
}

inline int wine_connect(int sockfd, const struct sockaddr *addr, unsigned int addrlen) {
    void* args[3];
    args[0] = reinterpret_cast<void*>(sockfd);
    args[1] = const_cast<struct sockaddr*>(addr);
    args[2] = reinterpret_cast<void*>(addrlen);
    return wine_socketcall(3, args);
}