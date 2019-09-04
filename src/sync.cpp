#include <cli.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <iostream>
#include <casher.h>
#include <boost/asio/streambuf.hpp>
#include <http.h>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace asio = boost::asio;

class session : public std::enable_shared_from_this<session> {
    asio::ip::tcp::socket socket;
    asio::streambuf read_buff;
    asio::streambuf write_buff;
    const std::string &doc_root;
    casher &cash;
public:
    explicit session(asio::ip::tcp::socket socket, const std::string &doc_root, casher &cash)
            : socket(std::move(socket)), doc_root(doc_root), cash{cash} {}

    void run() { on_connect(); }

    // read client's http request
    void on_connect() {
        boost::system::error_code ec;
        std::size_t read = asio::read(socket, read_buff, asio::transfer_at_least(1),ec);
        on_read(ec, read);
    }

    // callback for reading client's http request
    void on_read(boost::system::error_code &ec, std::size_t s) {
        if (!ec) {
            http::request req(read_buff);
            req.write_response(write_buff, doc_root, std::ref(cash));
            std::size_t wrote = asio::write(socket, write_buff, asio::transfer_all(), ec);
            on_write(ec, wrote);
        } else
            std::cerr << "Reading request error: " << ec.message() << std::endl;
    }

    // callback for writing http reply
    void on_write(boost::system::error_code ec, std::size_t s) {
        if (!ec)
            // close connection
            socket.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
        else
            std::cerr << "Writing reply error: " << ec.message() << std::endl;
    }
};


class listener {
    asio::ip::tcp::acceptor acceptor_;
    const std::string &doc_root_;
    asio::io_context &ioc_;
    casher cash;

public:
    listener(asio::io_context &ioc, const asio::ip::tcp::endpoint &endpoint, const std::string &doc_root,
             const bool no_cache)
            : acceptor_(ioc), doc_root_(doc_root), cash(!no_cache), ioc_(ioc) {
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
    void run() {
        if (!acceptor_.is_open())
            return;
        std::cout << "Running on http://" << acceptor_.local_endpoint().address() << ":"
                  << acceptor_.local_endpoint().port() << "/ (Press Ctrl+C to quit)" << std::endl;
        listen();
    }


    // accept connections
    [[noreturn]]void listen() {
        for (;;) {
            asio::ip::tcp::socket socket{ioc_};
            acceptor_.accept(socket);
            // initialize session
            auto sesh = std::make_shared<session>(std::move(socket), doc_root_, std::ref(cash));
            std::thread(&session::run, sesh).detach();
        }
    }


};

int main(int argc, char *argv[]) {
    try {
        cli cli_ob{argc, argv};
        // create ioservice shared between all threads
        asio::io_service ioc{1};
        // initialize and run listener
        listener listener_{ioc, asio::ip::tcp::endpoint{asio::ip::tcp::v4(),
                                                        cli_ob.get_port()}, cli_ob.get_directory(),
                           cli_ob.get_no_cache()};
        listener_.run();

        return 0;
    } catch (const std::exception &err) {
        err.what();
        return 1;
    }
}