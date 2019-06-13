#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/config.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <fstream>

using tcp = boost::asio::ip::tcp;

void
write_handler(boost::asio::ip::tcp::socket &socket, const boost::system::error_code &ec,
              std::size_t bytes_transferred) {
    if (!ec)
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    else
        std::cerr << "Writing error: " << ec.message() << std::endl;
}

void fail(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session> {
    tcp::socket socket;
    boost::asio::streambuf buff;
public:
    // Take ownership of the socket
    explicit
    session(
            tcp::socket socket)
            : socket(std::move(socket)) {
    }

    // Start the asynchronous operation
    void
    run() {
        do_read();
    }

    void
    do_read() {
        auto pThis = shared_from_this();
        // Read a request
        boost::asio::async_read(pThis->socket, pThis->buff, [pThis](const boost::system::error_code &e, std::size_t s) {
            std::ostringstream ss;
            ss << &pThis->buff;
            std::string string = ss.str();
            std::cout << string << std::endl;
        });
//        boost::asio::async_read_until(pThis->socket, pThis->buff, '\r', [pThis](const boost::system::error_code &e, std::size_t s) {
//            std::string line, ignore;
//
//            std::istream stream{&pThis->buff};
//
//            std::getline(stream, line, '\r');
//
//            std::getline(stream, ignore, '\n');
//            std::cout<<line<<std::endl;
//        });
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    listener(
            boost::asio::io_context &ioc,
            tcp::endpoint endpoint)
            : acceptor_(ioc), socket_(ioc) {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run() {
        if (!acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept() {
        acceptor_.async_accept(
                socket_,
                std::bind(
                        &listener::on_accept,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    void
    on_accept(boost::system::error_code ec) {
        if (ec) {
            fail(ec, "accept");
        } else {
            // Create the session and run it
            std::make_shared<session>(
                    std::move(socket_))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    // Check command line arguments.
    if (argc != 4) {
        std::cerr <<
                  "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
                  "Example:\n" <<
                  "    http-server-async 127.0.0.1 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    boost::asio::io_service ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(
            ioc,
            tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back(
                [&ioc] {
                    ioc.run();
                });
    ioc.run();

    return EXIT_SUCCESS;
}
