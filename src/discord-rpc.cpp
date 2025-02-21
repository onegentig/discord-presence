#include <discord-rpc.hpp>

#ifndef DISCORD_DISABLE_IO_THREAD
#include <condition_variable>
#include <thread>
#endif

#include "platform/platform.hpp"

#include "backoff.hpp"
#include "rpc-connection.hpp"

namespace discord {
    #ifdef DISCORD_DISABLE_IO_THREAD
    struct IOWorker {
        void start() {}
        void stop() {}
        void notify() {}
    };
    #else
    struct IOWorker {
        IOWorker() noexcept = default;
        ~IOWorker() noexcept { stop(); }

        void start() {
            m_running.store(true);
            m_thread = std::thread(
                [this]() {
                    const auto timeout = std::chrono::milliseconds(500);
                    auto& rpc = RPCManager::get();
                    rpc.update();
                    while (m_running.load()) {
                        std::unique_lock lock(m_waitForIO);
                        m_ioReady.wait_for(lock, timeout);
                        rpc.update();
                    }
                }
            );
        }

        void stop() {
            m_running.store(false);
            notify();
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        void notify() { m_ioReady.notify_one(); }

    private:
        std::thread m_thread{};
        std::atomic_bool m_running = true;
        std::mutex m_waitForIO{};
        std::condition_variable m_ioReady{};
    };
    #endif

    Presence& Presence::clear() noexcept {
        m_state.clear();
        m_details.clear();
        m_startTimestamp = 0;
        m_endTimestamp = 0;
        m_largeImageKey.clear();
        m_largeImageText.clear();
        m_smallImageKey.clear();
        m_smallImageText.clear();
        m_partyID.clear();
        m_partySize = 0;
        m_partyMax = 0;
        m_partyPrivacy = PartyPrivacy::Private;
        m_matchSecret.clear();
        m_joinSecret.clear();
        m_spectateSecret.clear();
        m_buttons[0] = Button{};
        m_buttons[1] = Button{};
        m_instance = false;
        return *this;
    }

    void Presence::refresh() const noexcept {
        RPCManager::get().refresh();
    }

    RPCManager& RPCManager::initialize() noexcept {
        if (m_initialized) {
            return *this;
        }

        m_ioWorker = new(std::nothrow) IOWorker();
        if (m_ioWorker) {
            m_ioWorker->start();
        }

        m_processID = platform::getProcessID();
        m_initialized = true;

        return *this;
    }

    RPCManager& RPCManager::shutdown() noexcept {
        if (!m_initialized) {
            return *this;
        }

        if (m_ioWorker) {
            delete m_ioWorker;
            m_ioWorker = nullptr;
        }

        Connection::get().close();
        m_initialized = false;

        return *this;
    }

    RPCManager& RPCManager::update() noexcept {
        if (!m_initialized) {
            return *this;
        }

        auto& conn = Connection::get();
        if (!conn.isOpen()) {
            if (std::chrono::system_clock::now() < m_nextConnect) {
                return *this;
            }

            updateReconnectTime();
            conn.open(m_clientID);
            return *this;
        }

        // reading
        do {
            std::string buffer;
            if (!conn.read(buffer)) {
                break;
            }

            // fmt::println("Received: {}", buffer);
        } while (true);

        // writing
        if (m_updatePresence.exchange(false)) {
            std::string presence;
            {
                std::lock_guard lock(m_queuePresenceMutex);
                presence = m_queuedPresence;
            }

            if (!conn.write(presence)) {
                // requeue
                std::lock_guard lock(m_queuePresenceMutex);
                m_queuedPresence = presence;
                m_updatePresence.exchange(true);
            }
        }

        return *this;
    }

    RPCManager& RPCManager::refresh() noexcept {
        // copy the presence to queue
        {
            std::lock_guard lock(m_queuePresenceMutex);
            serializePresence(m_queuedPresence, m_presence, m_processID, m_nonce++);
            m_updatePresence.exchange(true);
        }

        // notify the io worker
        if (m_ioWorker) { m_ioWorker->notify(); }

        return *this;
    }

    RPCManager& RPCManager::clearPresence() noexcept {
        m_presence.clear();

        // copy the presence to queue
        {
            std::lock_guard lock(m_queuePresenceMutex);
            serializeEmptyPresence(m_queuedPresence, m_processID, m_nonce++);
            m_updatePresence.exchange(true);
        }

        // notify the io worker
        if (m_ioWorker) { m_ioWorker->notify(); }

        return *this;
    }

    void RPCManager::updateReconnectTime() noexcept {
        m_nextConnect = std::chrono::system_clock::now() + std::chrono::milliseconds(Backoff::get().next());
    }
}
