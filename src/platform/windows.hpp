#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME
#define NOMINMAX
#include <Windows.h>

namespace discord::platform {
    inline size_t getProcessID() noexcept {
        return ::GetCurrentProcessId();
    }

    class PipeConnection {
        PipeConnection() noexcept = default;

    public:
        static PipeConnection& get() noexcept {
            static PipeConnection instance;
            return instance;
        }

        bool open() noexcept {
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

        bool close() noexcept {
            if (m_pipe != INVALID_HANDLE_VALUE) {
                ::CloseHandle(m_pipe);
                m_pipe = INVALID_HANDLE_VALUE;
                m_isOpen = false;
            }
            return true;
        }

        [[nodiscard]] bool isOpen() const noexcept {
            return m_isOpen;
        }

        bool write(const void* data, size_t length) const noexcept {
            if (length == 0) {
                return true;
            }

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

        bool read(void* data, size_t length) noexcept {
            if (!data) {
                return false;
            }

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

    private:
        HANDLE m_pipe = INVALID_HANDLE_VALUE;
        bool m_isOpen = false;
    };
}
