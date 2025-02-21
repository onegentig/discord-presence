#pragma once
#ifndef DISCORD_RPC_HPP
#define DISCORD_RPC_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace discord {
    class Connection;
    struct IOWorker;

    struct User {
        std::string id;
        std::string username;
        std::string discriminator;
        std::string global_name;
        std::string avatar;
        bool bot = false;
        uint64_t flags = 0;
        int premium_type = 0;
    };

    enum class PartyPrivacy : int32_t {
        Private = 0,
        Public  = 1,
    };

    class Presence {
    public:
        #define GENERATE_GETSET_LRVALUE(type, name, member) \
        Presence& set##name(type const& member) noexcept { m_##member = member; return *this; } \
        Presence& set##name(type&& member) noexcept { m_##member = std::move(member); return *this; }\
        type const& get##name() const noexcept { return m_##member; }

        #define GENERATE_GETSET_VALUE(type, name, member) \
        Presence& set##name(type member) noexcept { m_##member = member; return *this; } \
        type get##name() const noexcept { return m_##member; }

        #define GENERATE_GETSET_BUTTON(index) \
        Button& getButton##index() noexcept { return m_buttons[index - 1]; } \
        Button const& getButton##index() const noexcept { return m_buttons[index - 1]; } \
        Presence& setButton##index(Button const& button) noexcept { m_buttons[index - 1] = button; return *this; } \
        Presence& setButton##index(Button&& button) noexcept { m_buttons[index - 1] = std::move(button); return *this; } \
        Presence& setButton##index(std::string const& label, std::string const& url, bool enabled = true) noexcept { \
            m_buttons[index - 1].set(label, url, enabled); \
            return *this; \
        } \
        Presence& setButton##index(std::string&& label, std::string&& url, bool enabled = true) noexcept { \
            m_buttons[index - 1].set(std::move(label), std::move(url), enabled); \
            return *this; \
        }

        class Button {
        public:
            [[nodiscard]] bool isEnabled() const noexcept { return enabled; }
            [[nodiscard]] std::string const& getLabel() const noexcept { return label; }
            [[nodiscard]] std::string const& getURL() const noexcept { return url; }

            Button& setEnabled(bool enabled) noexcept {
                this->enabled = enabled;
                return *this;
            }

            Button& setLabel(std::string const& label) noexcept {
                this->label = label;
                return *this;
            }

            Button& setLabel(std::string&& label) noexcept {
                this->label = std::move(label);
                return *this;
            }

            Button& setURL(std::string const& url) noexcept {
                this->url = url;
                return *this;
            }

            Button& setURL(std::string&& url) noexcept {
                this->url = std::move(url);
                return *this;
            }

            Button& set(std::string const& label, std::string const& url, bool enabled = true) noexcept {
                this->label = label;
                this->url = url;
                this->enabled = enabled;
                return *this;
            }

            Button& set(std::string&& label, std::string&& url, bool enabled = true) noexcept {
                this->label = std::move(label);
                this->url = std::move(url);
                this->enabled = enabled;
                return *this;
            }

        private:
            bool enabled = false;
            std::string label;
            std::string url;
        };

        GENERATE_GETSET_LRVALUE(std::string, State, state)
        GENERATE_GETSET_LRVALUE(std::string, Details, details)
        GENERATE_GETSET_VALUE(int64_t, StartTimestamp, startTimestamp)
        GENERATE_GETSET_VALUE(int64_t, EndTimestamp, endTimestamp)
        GENERATE_GETSET_LRVALUE(std::string, LargeImageKey, largeImageKey)
        GENERATE_GETSET_LRVALUE(std::string, LargeImageText, largeImageText)
        GENERATE_GETSET_LRVALUE(std::string, SmallImageKey, smallImageKey)
        GENERATE_GETSET_LRVALUE(std::string, SmallImageText, smallImageText)
        GENERATE_GETSET_LRVALUE(std::string, PartyID, partyID)
        GENERATE_GETSET_VALUE(int32_t, PartySize, partySize)
        GENERATE_GETSET_VALUE(int32_t, PartyMax, partyMax)
        GENERATE_GETSET_VALUE(PartyPrivacy, PartyPrivacy, partyPrivacy)
        GENERATE_GETSET_LRVALUE(std::string, MatchSecret, matchSecret)
        GENERATE_GETSET_LRVALUE(std::string, JoinSecret, joinSecret)
        GENERATE_GETSET_LRVALUE(std::string, SpectateSecret, spectateSecret)
        GENERATE_GETSET_VALUE(bool, Instance, instance)

        GENERATE_GETSET_BUTTON(1)
        GENERATE_GETSET_BUTTON(2)

        #undef GENERATE_GETSET_BUTTON
        #undef GENERATE_GETSET_LRVALUE
        #undef GENERATE_GETSET_VALUE

        Presence& clear() noexcept;

        /// Calls the RPCManager::refresh() function to update the presence
        void refresh() const noexcept;

    private:
        std::string m_state;
        std::string m_details;
        int64_t m_startTimestamp = 0;
        int64_t m_endTimestamp = 0;
        std::string m_largeImageKey;
        std::string m_largeImageText;
        std::string m_smallImageKey;
        std::string m_smallImageText;
        std::string m_partyID;
        int32_t m_partySize = 0;
        int32_t m_partyMax = 0;
        PartyPrivacy m_partyPrivacy = PartyPrivacy::Private;
        std::string m_matchSecret;
        std::string m_joinSecret;
        std::string m_spectateSecret;
        std::array<Button, 2> m_buttons;
        bool m_instance = false;
    };

    class RPCManager {
    private:
        // prevent construction from outside
        RPCManager() noexcept = default;
        ~RPCManager() noexcept { shutdown(); }

        friend class Connection;

    public:
        static RPCManager& get() noexcept {
            static RPCManager instance;
            return instance;
        }

        // prevent copying/moving
        RPCManager(RPCManager const&) = delete;
        RPCManager(RPCManager&&) = delete;
        RPCManager& operator=(RPCManager const&) = delete;
        RPCManager& operator=(RPCManager&&) = delete;

        /// Initializes the RPC manager and starts the IO worker (if not disabled)
        RPCManager& initialize() noexcept;

        /// Disconnects the RPC manager and stops the IO worker (if not disabled)
        RPCManager& shutdown() noexcept;

        /// Sends a heartbeat to the Discord client
        /// @note This function is called automatically by the IO worker thread (if not disabled)
        RPCManager& update() noexcept;

        /// Send a new presence to the Discord client
        RPCManager& refresh() noexcept;

        /// Get current rich presence information. You can use this to access the builder directly.
        Presence& getPresence() noexcept { return m_presence; }

        /// Clear the current rich presence information. Calls refresh() automatically.
        RPCManager& clearPresence() noexcept;

        #define GENERATE_SETTER_LRVALUE(type, name, member) \
        RPCManager& name(type const& member) noexcept { m_##member = member; return *this; } \
        RPCManager& name(type&& member) noexcept { m_##member = std::move(member); return *this; }

        GENERATE_SETTER_LRVALUE(std::string, setClientID, clientID)
        GENERATE_SETTER_LRVALUE(Presence, setPresence, presence)
        GENERATE_SETTER_LRVALUE(std::function<void(User const&)>, onReady, onReady)
        GENERATE_SETTER_LRVALUE(std::function<void(int, std::string_view)>, onDisconnected, onDisconnected)
        GENERATE_SETTER_LRVALUE(std::function<void(int, std::string_view)>, onErrored, onErrored)
        GENERATE_SETTER_LRVALUE(std::function<void(std::string_view)>, onJoinGame, onJoinGame)
        GENERATE_SETTER_LRVALUE(std::function<void(std::string_view)>, onSpectateGame, onSpectateGame)
        GENERATE_SETTER_LRVALUE(std::function<void(User const&)>, onJoinRequest, onJoinRequest)

        #undef GENERATE_SETTER_LRVALUE

    private:
        void invokeOnReady(User const& user) const noexcept {
            if (m_onReady) { m_onReady(user); }
        }

        void invokeOnDisconnected(int errcode, std::string_view message) const noexcept {
            if (m_onDisconnected) { m_onDisconnected(errcode, message); }
        }

        void invokeOnErrored(int errcode, std::string_view message) const noexcept {
            if (m_onErrored) { m_onErrored(errcode, message); }
        }

        void invokeOnJoinGame(std::string_view joinSecret) const noexcept {
            if (m_onJoinGame) { m_onJoinGame(joinSecret); }
        }

        void invokeOnSpectateGame(std::string_view spectateSecret) const noexcept {
            if (m_onSpectateGame) { m_onSpectateGame(spectateSecret); }
        }

        void invokeOnJoinRequest(User const& user) const noexcept {
            if (m_onJoinRequest) { m_onJoinRequest(user); }
        }

        void updateReconnectTime() noexcept;

    private:
        // User settings
        std::string m_clientID;
        Presence m_presence{};
        std::function<void(User const&)> m_onReady;
        std::function<void(int, std::string_view)> m_onDisconnected;
        std::function<void(int, std::string_view)> m_onErrored;
        std::function<void(std::string_view)> m_onJoinGame;
        std::function<void(std::string_view)> m_onSpectateGame;
        std::function<void(User const&)> m_onJoinRequest;

        // State
        bool m_initialized = false;

        // Internal
        IOWorker* m_ioWorker = nullptr;
        std::chrono::time_point<std::chrono::system_clock> m_nextConnect = std::chrono::system_clock::now();
        size_t m_processID = 0;
        int m_nonce = 1;
        std::string m_queuedPresence{};
        std::mutex m_queuePresenceMutex{};
        std::atomic_bool m_updatePresence = false;
    };
}

#endif // DISCORD_RPC_HPP
