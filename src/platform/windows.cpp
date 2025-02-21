#include "windows.hpp"
#include <fmt/format.h>

#ifndef DISCORD_DISABLE_WINE_LAYER
#if defined(_M_IX86) || defined(_X86_) // x86
#include "wine_x86.hpp"
#elif defined(__GNUC__) || defined(__clang__) // x86_64 (GCC/Clang)
#include "wine_gcc.hpp"
#elif defined(_MSC_VER) // x86_64 (MSVC)
int wine_close(uint32_t fd);
int wine_open(const char* path, int flags, int mode);
int wine_read(uint32_t fd, void* buffer, uint32_t count);
int wine_socket(int domain, int type, int protocol);
int wine_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int wine_send(int sockfd, const void* buf, size_t len, int flags);
int wine_recv(int sockfd, void* buf, size_t len, int flags);
int wine_fcntl(int fd, int cmd, ...);
#endif
#endif

namespace discord::platform {
    #ifndef DISCORD_DISABLE_WINE_LAYER
    HMODULE getNTDLL() noexcept {
        static HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
        return ntdll;
    }

    bool isWine() noexcept {
        static auto isWine = []() {
            auto ntdll = getNTDLL();
            if (!ntdll) {
                return false;
            }

            return ::GetProcAddress(ntdll, "wine_get_version") != nullptr;
        }();
        return isWine;
    }

    const char* getenv(const char* name) noexcept {
        static char buffer[1024 * 1024]{};
        static const char* end = nullptr;

        if (!end) {
            auto fd = wine_open("/proc/self/environ", 0, 0);
            if (fd < 0) {
                return nullptr;
            }

            auto n = wine_read(fd, buffer, sizeof(buffer));
            if (n < 0) {
                wine_close(fd);
                return nullptr;
            }

            wine_close(fd);
            end = buffer + n;
        }

        auto namelen = ::strlen(name);

        for (const char* ptr = buffer; ptr < end;) {
            if (!::strncmp(ptr, name, namelen) && ptr[namelen] == '=') {
                return ptr + namelen + 1;
            }

            for (; *ptr && ptr < end; ++ptr);
            ++ptr;
        }

        return nullptr;
    }

    const char* getTempPath() noexcept {
        static const char* path = []() {
            const char* tmp = getenv("XDG_RUNTIME_DIR");
            tmp = tmp ? tmp : getenv("TMPDIR");
            tmp = tmp ? tmp : getenv("TMP");
            tmp = tmp ? tmp : getenv("TEMP");
            return tmp ? tmp : "/tmp";
        }();
        return path;
    }

    wine::WineConnector& wine::WineConnector::get() noexcept {
        static WineConnector instance;
        return instance;
    }

    bool wine::WineConnector::open() noexcept {
        if (isOpen() || m_socket != -1) {
            return false;
        }

        m_socket = wine_socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_socket == -1) {
            return false;
        }

        wine_fcntl(m_socket, F_SETFL, O_NONBLOCK);
        for (int i = 0; i < 10; ++i) {
            fmt::format_to(m_address.sun_path, "{}/discord-ipc-{}", getTempPath(), i);
            if (wine_connect(m_socket, reinterpret_cast<sockaddr*>(&m_address), sizeof(m_address)) == 0) {
                setOpen(true);
                return true;
            }
        }

        m_socket = -1;
        return false;
    }

    bool wine::WineConnector::close() noexcept {
        if (!isOpen() || m_socket == -1) {
            return false;
        }

        wine_close(m_socket);
        m_socket = -1;
        setOpen(false);
        return true;
    }

    bool wine::WineConnector::write(const void* data, size_t length) noexcept {
        if (!isOpen() || m_socket == -1) {
            return false;
        }

        auto written = wine_write(m_socket, data, length);
        if (written < 0) {
            this->close();
            return false;
        }

        return static_cast<size_t>(written) == length;
    }

    bool wine::WineConnector::read(void* data, size_t length) noexcept {
        if (!isOpen() || m_socket == -1) {
            return false;
        }

        auto received = wine_read(m_socket, data, length);
        if (received < 0) {
            if (received == -EAGAIN || received == -EWOULDBLOCK) {
                return false;
            }

            this->close();
            return false;
        }

        if (received == 0) {
            this->close();
            return false;
        }

        return static_cast<size_t>(received) == length;
    }

    bool wine::WineConnector::isOpen() noexcept {
        return PipeConnection::get().isOpen();
    }

    void wine::WineConnector::setOpen(bool open) noexcept {
        PipeConnection::get().m_isOpen = open;
    }

    #endif

    size_t getProcessID() noexcept {
        return ::GetCurrentProcessId();
    }

    PipeConnection& PipeConnection::get() noexcept {
        static PipeConnection instance;
        return instance;
    }

    bool PipeConnection::open() noexcept {
        if (m_isOpen) {
            return false;
        }

        #ifndef DISCORD_DISABLE_WINE_LAYER
        if (isWine()) {
            return wine::WineConnector::get().open();
        }
        #endif

        wchar_t pipeName[] = L"\\\\?\\pipe\\discord-ipc-0";
        constexpr size_t pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
        while (true) {
            m_pipe = ::CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (m_pipe != INVALID_HANDLE_VALUE) {
                m_isOpen = true;
                return true;
            }

            auto error = ::GetLastError();
            if (error == ERROR_FILE_NOT_FOUND) {
                if (pipeName[pipeDigit] < L'9') {
                    pipeName[pipeDigit]++;
                    continue;
                }
            } else if (error == ERROR_PIPE_BUSY) {
                if (!WaitNamedPipeW(pipeName, 10000)) {
                    return false;
                }
                continue;
            }

            return false;
        }
    }

    bool PipeConnection::close() noexcept {
        #ifndef DISCORD_DISABLE_WINE_LAYER
        if (isWine()) {
            return wine::WineConnector::get().close();
        }
        #endif

        if (m_pipe != INVALID_HANDLE_VALUE) {
            ::CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
            m_isOpen = false;
        }
        return true;
    }

    bool PipeConnection::write(const void* data, size_t length) const noexcept {
        if (length == 0) {
            return true;
        }

        #ifndef DISCORD_DISABLE_WINE_LAYER
        if (isWine()) {
            return wine::WineConnector::get().write(data, length);
        }
        #endif

        if (m_pipe == INVALID_HANDLE_VALUE) {
            return false;
        }

        if (!data) {
            return false;
        }

        const auto bytesToWrite = static_cast<DWORD>(length);
        DWORD bytesWritten = 0;
        if (!::WriteFile(m_pipe, data, bytesToWrite, &bytesWritten, nullptr)) {
            return false;
        }
        return bytesWritten == bytesToWrite;
    }

    bool PipeConnection::read(void* data, size_t length) noexcept {
        if (!data) {
            return false;
        }

        #ifndef DISCORD_DISABLE_WINE_LAYER
        if (isWine()) {
            return wine::WineConnector::get().read(data, length);
        }
        #endif

        if (m_pipe == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesRead = 0;
        if (!::PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &bytesRead, nullptr)) {
            this->close();
            return false;
        }

        if (bytesRead < length) {
            return false;
        }

        if (!::ReadFile(m_pipe, data, length, &bytesRead, nullptr)) {
            this->close();
            return false;
        }

        return true;
    }
}
