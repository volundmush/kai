#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast.hpp>
#include <mutex>

namespace net {
    class Connection;
    using namespace std::chrono_literals;
    using namespace boost::asio;
    using namespace boost::beast;

    class Link {
    public:
        explicit Link(boost::beast::websocket::stream<boost::beast::tcp_stream> ws);

        awaitable<void> run();
        void stop();

    protected:
        awaitable<void> runReader();
        awaitable<void> runWriter();
        awaitable<void> runPinger();
        awaitable<void> createUpdateClient(const nlohmann::json &j);
        boost::beast::websocket::stream<boost::beast::tcp_stream> conn;
        bool is_stopped;
    };

    template<typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    using JsonChannel = Channel<nlohmann::json>;

    extern std::unique_ptr<io_context> io;

    extern std::unique_ptr<signal_set> signals;

    extern std::unique_ptr<Channel<nlohmann::json>> linkChannel;
    extern boost::asio::ip::tcp::endpoint thermiteEndpoint;

    extern std::unique_ptr<Link> link;

    enum class DisconnectReason {
        // In these first two examples, the connection is dead on the portal and we have been informed of such.
        ConnectionLost = 0,
        ConnectionClosed = 1,
        // In the remaining enums, the connection is still alive on the portal, but we are disconnecting for some reason.
        // We must inform the portal.
        GameLogoff = 2,
    };

    extern std::mutex connectionsMutex, pendingConnectionsMutex;
    extern std::map<int64_t, std::shared_ptr<Connection>> connections;
    extern std::set<int64_t> pendingConnections;

    extern std::unordered_map<int64_t, DisconnectReason> deadConnections;

    awaitable<void> runLinkManager();

    enum class Protocol : uint8_t {
        Telnet = 0,
        WebSocket = 1
    };

    enum class ColorType : uint8_t {
        NoColor = 0,
        Standard = 1,
        Xterm256 = 2,
        TrueColor = 3
    };

    struct Message {
        Message();
        explicit Message(const nlohmann::json& j);
        std::string cmd;
        nlohmann::json args;
        nlohmann::json kwargs;
        nlohmann::json serialize() const;
    };

    struct ProtocolCapabilities {
        Protocol protocol{Protocol::Telnet};

        std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
        std::string hostAddress = "UNKNOWN";
        std::string encoding;
        std::vector<std::string> hostNames{};
        ColorType colorType = ColorType::NoColor;
        int16_t hostPort{0};

        bool encryption = false;
        bool utf8 = false;
        int width = 80, height = 52;
        bool gmcp = false, msdp = false, mssp = false, mxp = false;
        bool mccp2 = false, mccp2_active = false, mccp3 = false, mccp3_active = false;
        bool ttype = false, naws = false, sga = false, linemode = false;
        bool force_endline = false, oob = false, tls = false;
        bool screen_reader = false, mouse_tracking = false, vt100 = false;
        bool osc_color_palette = false, proxy = false, mnes = false;

        void deserialize(const nlohmann::json& j);
        nlohmann::json serialize();
        std::string protocolName();
    };

    class Connection;

    class ConnectionParser {
    public:
        explicit ConnectionParser(const std::shared_ptr<Connection>& conn) : conn(conn) {}
        virtual ~ConnectionParser() = default;
        virtual void handleMessage(const Message &m);
        virtual void parse(const std::string &txt) = 0;
        virtual void start();
        virtual void close();
    protected:
        void sendText(const std::string &txt);
        std::shared_ptr<Connection> conn;
    };

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        explicit Connection(int64_t connId);
        void sendMessage(const Message &msg);
        void sendText(const std::string &messg);
        void handleMessage(const Message &m);
        void onHeartbeat(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();

        void cleanup(DisconnectReason reason);

        void setParser(ConnectionParser *p);

        int64_t connId{};
        std::weak_ptr<GameObject> account;
        int64_t adminLevel{0};
        std::shared_ptr<PlayView> playview;

        // Some time structs to handle when we received connections.
        // These probably need some updating on this and Thermite side...
        std::chrono::system_clock::time_point connected{};
        std::chrono::steady_clock::time_point connectedSteady{}, lastActivity{}, lastMsg{};

        // This is embedded for ease of segmentation but this struct isn't
        // actually used anywhere else.
        ProtocolCapabilities capabilities{};

        JsonChannel fromLink;
        std::unique_ptr<ConnectionParser> parser;

    };


}