// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "http.h"
#include "casher.h"

namespace asio = boost::asio;
namespace po = boost::program_options;

class session : public std::enable_shared_from_this<session> {
    asio::ip::tcp::socket socket;
    asio::streambuf read_buff;
    asio::streambuf write_buff;
    const std::string &doc_root;
    casher& cash;
public:
    explicit session(asio::ip::tcp::socket socket, const std::string &doc_root, casher &cash)
            : socket(std::move(socket)), doc_root(doc_root), cash{cash} {}

    void run() { on_connect(); }

    // read client's http request
    void on_connect() {
        asio::async_read(socket, read_buff, asio::transfer_at_least(1),
                         std::bind(&session::on_read, shared_from_this(), std::placeholders::_1,
                                   std::placeholders::_2));
    }

    // callback for reading client's http request
    void on_read(boost::system::error_code ec, std::size_t s) {
        if (!ec) {
            http::request req(read_buff);
            req.write_response(write_buff, doc_root, std::ref(cash));
            asio::async_write(socket, write_buff,
                              std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                        std::placeholders::_2));
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

class listener : public std::enable_shared_from_this<listener> {
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket socket_;
    const std::string &doc_root_;
    casher cash;

public:
    listener(asio::io_context &ioc, const asio::ip::tcp::endpoint &endpoint, const std::string &doc_root)
            : acceptor_(ioc), socket_(ioc), doc_root_(doc_root), cash(true) {
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
        do_accept();
    }

    // accept connections
    void do_accept() {
        acceptor_.async_accept(socket_, std::bind(&listener::on_accept, shared_from_this(), std::placeholders::_1));
    }

    // callback for establishing connection
    void on_accept(boost::system::error_code ec) {
        if (!ec)
            // initialize session
            std::make_shared<session>(std::move(socket_), doc_root_,std::ref(cash))->run();
        else
            std::cerr << "Accepting connection error: " << ec.message() << std::endl;
        // accept next connection
        do_accept();
    }
};

int main(int argc, char *argv[]) {
    // define options to be shown on --help
    po::options_description basic_options("Options");
    basic_options.add_options()
            ("help", "print help message")
            ("port,p", po::value<uint16_t>(), "specify port number")
            ("threads,t", po::value<int>()->default_value(2), "specify number of threads to use");

    // define hidden options
    po::options_description hidden_options("Hidden options");
    hidden_options.add_options()
            ("directory,d", po::value<std::string>(), "directory to serve from");

    po::options_description options;
    options.add(basic_options).add(hidden_options);

    // define positional options
    po::positional_options_description positional;
    positional.add("directory", 1);

    po::variables_map vm;
    auto parsed = po::command_line_parser(argc, argv).options(options).positional(positional).run();
    po::store(parsed, vm);

    // show help message
    if (vm.count("help")) {
        std::cout << basic_options << std::endl;
        return 0;
    }

    // port is not specified
    if (!vm.count("port")) {
        std::cout << "Port to use is not specified" << std::endl;
        return 1;
    }

    // directory is not specified
    if (!vm.count("directory")) {
        std::cout << "Directory to serve from is not specified" << std::endl;
        return 1;
    }

    auto threads = std::max(1, vm["threads"].as<int>());

    // create ioservice shared between all threads
    asio::io_service ioc{threads};
    // initialize and run listener
    std::make_shared<listener>(ioc, asio::ip::tcp::endpoint{asio::ip::tcp::v4(),
                                                            vm["port"].as<uint16_t>()},
                               vm["directory"].as<std::string>())->run();

    // run ioservice in every thread
    std::vector<std::thread> thread_vector;
    thread_vector.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        thread_vector.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    return 0;
}
