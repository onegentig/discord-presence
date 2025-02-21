#pragma once
#ifndef DISCORD_WINDOWS_HPP
#define DISCORD_WINDOWS_HPP

#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME
#define NOMINMAX
#include <cstdint>
#include <Windows.h>

// only enable wine layer on x86 and x86_64
#if !defined(_M_IX86) && !defined(_X86_) && !defined(_M_X64) && !defined(_AMD64_)
#define DISCORD_DISABLE_WINE_LAYER 1
#else

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

#define AF_UNIX     1
#define SOCK_STREAM 1
#define F_SETFL     4
#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_CREAT     00000100
#define O_APPEND    00002000
#define O_NONBLOCK  00004000
#define BUFSIZE 2048

using socklen_t = uint32_t;
using ssize_t = intptr_t;

#endif

namespace discord::platform {
    // wine compatibility layer
    #ifndef DISCORD_DISABLE_WINE_LAYER
    namespace wine {
        class WineConnector {
            WineConnector() noexcept {
                m_address.sun_family = AF_UNIX;
            }

        public:
            static WineConnector& get() noexcept;
            bool open() noexcept;
            bool close() noexcept;

            bool write(const void* data, size_t length) noexcept;
            bool read(void* data, size_t length) noexcept;

            [[nodiscard]] static bool isOpen() noexcept;
            static void setOpen(bool open) noexcept;

        private:
            sockaddr_un m_address{};
            int m_socket = -1;
        };
    }
    #endif

    size_t getProcessID() noexcept;

    class PipeConnection {
        PipeConnection() noexcept = default;
        friend class wine::WineConnector;

    public:
        static PipeConnection& get() noexcept;

        bool open() noexcept;
        bool close() noexcept;

        bool write(const void* data, size_t length) const noexcept;
        bool read(void* data, size_t length) noexcept;

        [[nodiscard]] bool isOpen() const noexcept { return m_isOpen; }

    private:
        HANDLE m_pipe = INVALID_HANDLE_VALUE;
        bool m_isOpen = false;
    };
}

#endif // DISCORD_WINDOWS_HPP
