#pragma once

// Inline assembly for Wine on x86_64 (AT&T syntax)

inline int wine_close(uint32_t fd) {
    int ret;
    __asm (
        "mov $3, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(fd)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline int wine_open(const char* path, int flags, int mode) {
    int ret;
    __asm (
        "mov $2, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline ssize_t wine_read(uint32_t fd, void* buffer, size_t count) {
    ssize_t ret;
    __asm (
        "mov $0, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(fd), "S"(buffer), "d"(count)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline ssize_t wine_write(uint32_t fd, const void* buf, size_t count) {
    ssize_t ret;
    __asm (
        "mov $1, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline int wine_socket(int domain, int type, int protocol) {
    int ret;
    __asm (
        "mov $41, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(domain), "S"(type), "d"(protocol)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline int wine_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int ret;
    __asm (
        "mov $42, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(sockfd), "S"(addr), "d"(addrlen)
        : "rcx", "r11", "memory"
    );
    return ret;
}

inline int wine_fcntl(int fd, int cmd, ...) {
    va_list args;
    va_start(args, cmd);
    int ret;
    __asm (
        "mov $72, %%rax\n"
        "syscall"
        : "=a"(ret)
        : "D"(fd), "S"(cmd), "d"(va_arg(args, int))
        : "rcx", "r11", "memory"
    );
    va_end(args);
    return ret;
}