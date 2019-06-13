#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/config.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <QString>
#include <QVector>

using tcp = boost::asio::ip::tcp;

void fail(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

class session : public std::enable_shared_from_this<session> {
    tcp::socket socket;
    boost::asio::streambuf buff;
public:
    explicit session(tcp::socket socket)
            : socket(std::move(socket)) {}

    void run() {
        do_read();
    }

    void do_read() {
        auto pThis = shared_from_this();
        boost::asio::async_read(pThis->socket, pThis->buff,
                                boost::asio::transfer_at_least(1),
                                std::bind(&session::handle_http, pThis, std::placeholders::_1, std::placeholders::_2));
    }

    void handle_http(boost::system::error_code ec, std::size_t s) {
        auto pThis = shared_from_this();
        auto list = QString::fromStdString(
                std::string((std::istreambuf_iterator<char>(&buff)), std::istreambuf_iterator<char>())).split(" ");
        if (list[0] != "GET") {
            boost::asio::async_write(pThis->socket, boost::asio::buffer("HTTP/1.1 501 NOT IMPLEMENTED\n\n"),
                                     std::bind(&session::write_handler, pThis, std::placeholders::_1,
                                               std::placeholders::_2));
        } else {
            if (list[1] == "/")
                list[1] = "/index.html";
            list[1] = "../pages" + list[1];
            if (!boost::filesystem::exists(list[1].toStdString())) {
                boost::asio::async_write(pThis->socket, boost::asio::buffer("HTTP/1.1 404 NOT FOUND\n\n"),
                                         std::bind(&session::write_handler, pThis, std::placeholders::_1,
                                                   std::placeholders::_2));
            } else {
                std::ifstream file(list[1].toStdString());
                auto ss = std::ostringstream{};
                ss << file.rdbuf();
                auto body = ss.str();
                boost::asio::async_write(pThis->socket, boost::asio::buffer("HTTP/1.1 200 OK\n\n" + body),
                                         std::bind(&session::write_handler, pThis, std::placeholders::_1,
                                                   std::placeholders::_2));
            }

        }
    }

    void write_handler(boost::system::error_code ec, std::size_t s) {
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    }
};

class listener : public std::enable_shared_from_this<listener> {
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    listener(boost::asio::io_context &ioc, tcp::endpoint endpoint)
            : acceptor_(ioc), socket_(ioc) {
        boost::system::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    void run() {
        if (!acceptor_.is_open())
            return;
        do_accept();
    }

    void do_accept() {
        acceptor_.async_accept(socket_, std::bind(&listener::on_accept, shared_from_this(), std::placeholders::_1));
    }

    void on_accept(boost::system::error_code ec) {
        if (ec) {
            fail(ec, "accept");
        } else {
            std::make_shared<session>(std::move(socket_))->run();
        }

        do_accept();
    }
};

//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: http-server-async <address> <port> <doc_root> <threads>\n" << "Example:\n"
                  << "    http-server-async 127.0.0.1 8080 1\n";
        return 1;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::stoi(argv[2]));
    auto const threads = std::max<int>(1, std::stoi(argv[3]));

    boost::asio::io_service ioc{threads};

    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    return 0;
}
