#include <iostream>

#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <set>
#include <deque>
#include <chrono>
#include <cassert>

#include "gl_draw.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::atomic_uint32_t sid = 0;

struct packet
{
    std::string textBuffer;
    std::vector<uint8_t> binaryBuffer;
    bool text = false;
};

using shared_packet = std::shared_ptr<packet>;


static explorer* explorer_ptr {nullptr};

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket&& socket) : 
        m_id(++sid), 
        m_ws(std::move(socket))
    {
        // set suggested timeout settings for the websocket
        m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        // set a decorator to change the Server of the handshake
        m_ws.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + "ws-server");
            })
        );

        std::cout << "Created session " << m_id << '\n';
    }

    void run()
    {
        m_ws.async_accept(beast::bind_front_handler(&Session::onAccept, shared_from_this()));
    }

    void onAccept(beast::error_code e)
    {
        if (e) return fail(e, "accept");
        doRead();
    }

    void doRead()
    {
        m_ws.async_read(m_readBuf, beast::bind_front_handler(&Session::onRead, shared_from_this()));
    }

    void onRead(beast::error_code e, size_t /*length*/)
    {
        if (e == websocket::error::closed)
        {
            return close();
        }

        if (e)
        {
            return fail(e, "read");
        }

        if (!m_ws.got_text())
        {
            std::fstream file("file.png", std::ios_base::binary | std::ios_base::out);

            auto rpacket = std::make_shared<packet>();

            auto data = reinterpret_cast<const uint8_t*>(m_readBuf.cdata().data());

            size_t size = m_readBuf.size();

            for (size_t i = 0; i < size; ++i)
            {
                file << data[i];
            }

            file.close();

            std::cout << "Receive blob of size: " << size << '\n';
        }
        else
        {
            auto rpacket = std::make_shared<packet>();
            rpacket->text = true;
            auto data = reinterpret_cast<const char*>(m_readBuf.cdata().data());
            rpacket->textBuffer.assign(data, data + m_readBuf.size());

            onReceive(rpacket);
        }

        m_readBuf.clear();
        doRead(); // read loop
    }

    void onReceive(const shared_packet& packet)
    {
        std::stringstream ssjson(packet->textBuffer);

        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ssjson, pt); 

        const auto dcommand = pt.get_child_optional("command");
        const auto dvalue   = pt.get_child_optional("value");

        if (explorer_ptr && dcommand && dvalue)
        {
            const auto command = dcommand.get().get_value<std::string>();
            const auto value   = dvalue.get().get_value<std::string>();

            std::cout << "command : " << command << '\n';
            std::cout << "value   : " << value << '\n';

            if (command == "min_layer")
            {
                explorer_ptr->min_layer = (size_t)std::stoull(value);
                explorer_ptr->index = 0;
            }
            else if (command == "max_layer")
            {
                explorer_ptr->max_layer = (size_t)std::stoull(value);
                explorer_ptr->index = 0;
            }
        }

        std::cout << "Received: " << packet->textBuffer << '\n';

        //send(packet);
    }

    void send(const shared_packet& packet)
    {
        m_writeQueue.emplace_back(packet);
        if (m_writeQueue.size() > 1) return; // we're already writing

        doWrite();
    }

    void doWrite()
    {
        assert(!m_writeQueue.empty());

        auto& packet = m_writeQueue.front();
        m_ws.text(packet->text);
        auto handler = beast::bind_front_handler(&Session::onWrite, shared_from_this());
        if (packet->text) m_ws.async_write(net::buffer(packet->textBuffer), std::move(handler));
        else m_ws.async_write(net::buffer(packet->binaryBuffer), std::move(handler));
    }

    void onWrite(beast::error_code e, std::size_t)
    {
        if (e) return fail(e, "write");

        m_writeQueue.pop_front();
        if (m_writeQueue.empty()) return;

        doWrite();
    }

    void fail(beast::error_code e, const char* source)
    {
        std::cerr << "Session " << m_id << " error: " << e << " in " << source << '\n';
    }

    void close()
    {
        std::cout << "Session " << m_id << " closed \n";
    }

private:
    const uint32_t m_id;
    websocket::stream<tcp::socket> m_ws;

    // io
    beast::flat_buffer m_readBuf, m_writeBuf;

    std::deque<shared_packet> m_writeQueue;
};

class Server
{
public:
    Server(tcp::endpoint endpoint) : 
        m_context(1),
        m_acceptor(m_context, endpoint)
    {;}

    int run()
    {
        doAccept();
        m_context.run();

        return 0;
    }

    void doAccept()
    {
        m_acceptor.async_accept(beast::bind_front_handler(&Server::onAccept, this));
    }

    void onAccept(beast::error_code error, tcp::socket socket)
    {
        if (error)
        {
            std::cerr << "Server::onAccept error: " << error << '\n';
            return;
        }

        auto session = std::make_shared<Session>(std::move(socket));

        session->run();

        doAccept();
    }

private:
    net::io_context m_context;
    tcp::acceptor m_acceptor;
};


void start_server(explorer* ex)
{
    const auto address = boost::asio::ip::tcp::v4(); // net::ip::make_address(argAddr);
    const uint16_t port = 7654;

    explorer_ptr = ex;

    Server server(tcp::endpoint(address, port));

    server.run();
}