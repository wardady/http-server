#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/coroutine.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "http.h"
#include "casher.h"
#include "cli.h"

namespace asio = boost::asio;

class session : public std::enable_shared_from_this<session>, asio::coroutine {
    asio::ip::tcp::socket socket;
    asio::streambuf read_buff;
    asio::streambuf write_buff;
    const std::string &doc_root;
    casher &cash;
public:
    explicit session(asio::ip::tcp::socket socket, const std::string &doc_root, casher &cash)
            : socket(std::move(socket)), doc_root(doc_root), cash{cash} {}

    void run(const boost::system::error_code &ec = boost::system::error_code(), std::size_t s = 0) {
        BOOST_ASIO_CORO_REENTER(this) {
            BOOST_ASIO_CORO_YIELD asio::async_read(socket, read_buff, asio::transfer_at_least(1), std::bind(&session::run, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (ec)
                std::cerr << "Reading request error: " << ec.message() << std::endl;
            BOOST_ASIO_CORO_YIELD {
                http::request req(read_buff);
                req.write_response(write_buff, doc_root,std::ref(cash));
            };
            BOOST_ASIO_CORO_YIELD asio::async_write(socket, write_buff, std::bind(&session::run, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
            if (ec)
                std::cerr << "Writing reply error: " << ec.message() << std::endl;
            BOOST_ASIO_CORO_YIELD socket.shutdown(asio::ip::tcp::socket::shutdown_send);
        }
    }
};

class listener : public std::enable_shared_from_this<listener>, asio::coroutine {
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
    const std::string &doc_root_;
    casher cash;

public:
    listener(asio::io_context &ioc, const asio::ip::tcp::endpoint &endpoint, const std::string &doc_root,
             const bool no_cache)
            : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), cash(!no_cache) {
        boost::system::error_code ec;

        // initialize acceptor by endpoint
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.bind(endpoint, ec);
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);

        if (ec) {
            std::cerr << "Listener creation error: " << ec.message() << std::endl;
            return;
        }
    }

    // start accepting connections
    void run(const boost::system::error_code &ec = boost::system::error_code()) {
        std::cout << "Running on http://" << acceptor_.local_endpoint().address() << ":"
                  << acceptor_.local_endpoint().port() << "/ (Press Ctrl+C to quit)" << std::endl;
        if (!ec) {
            BOOST_ASIO_CORO_REENTER(this)
                for (;;) {
                    BOOST_ASIO_CORO_YIELD acceptor_.async_accept(socket_, std::bind(&listener::run, shared_from_this(), std::placeholders::_1));
                    BOOST_ASIO_CORO_YIELD std::make_shared<session>(std::move(socket_), doc_root_, std::ref(cash))->run();
                }
        }
    }
};

int main(int argc, char *argv[]) {
    cli cli_ob{argc, argv};
    // create ioservice shared between all threads
    asio::io_service ioc{cli_ob.get_threads()};
    // initialize and run listener
    std::make_shared<listener>(ioc, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), cli_ob.get_port()}, cli_ob.get_directory(), cli_ob.get_no_cache())->run();

    // run ioservice in every thread
    std::vector<std::thread> thread_vector;
    thread_vector.reserve(cli_ob.get_threads() - 1);
    for (auto i = cli_ob.get_threads() - 1; i > 0; --i)
        thread_vector.emplace_back([&ioc] { ioc.run(); });
    ioc.run();
    for (auto i = cli_ob.get_threads() - 1; i > 0; --i)
        thread_vector[i].join();
    return 0;
}
