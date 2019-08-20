// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

constexpr char msg_404_buffer[] = "HTTP/1.1 404 NOT FOUND\n\n <html><body><h1>404 Not Found</h1></body></html>";
constexpr char msg_501_buffer[] = "HTTP/1.1 501 NOT IMPLEMENTED \n\n <html><body><h1>501 Not Implemented</h1></body></html>";

namespace asio = boost::asio;
namespace po = boost::program_options;

class session : public std::enable_shared_from_this<session> {
    asio::ip::tcp::socket socket;
    asio::streambuf buff;
    const std::string &doc_root;
    std::string send_ok_response;
public:
    explicit session(asio::ip::tcp::socket socket, const std::string &doc_root)
            : socket(std::move(socket)), doc_root(doc_root) {}

    void run() { on_connect(); }

    // read client's http request
    void on_connect() {
        asio::async_read(socket, buff, asio::transfer_at_least(1),
                                std::bind(&session::on_read, shared_from_this(), std::placeholders::_1,
                                          std::placeholders::_2));
    }

    // callback for reading client's http request
    void on_read(boost::system::error_code ec, std::size_t s) {
        if (!ec) {
            // split request by space
            std::vector<std::string> list;
            // Взагалі, тут копіювання може й зайве, але boost::iterator_range щось капризує, то поки, для економії часу, так.
            auto input_string = std::string(std::istreambuf_iterator<char>(&buff), std::istreambuf_iterator<char>());
            boost::split(list, input_string, boost::is_any_of(" "));

            if (list[0] != "GET") {
                // only GET requests are supported
                asio::async_write(socket, asio::buffer( msg_501_buffer ),
                                         std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                                   std::placeholders::_2));
            } else {
                std::string& url_file_path = list[1];
                // if no file requested return index.html
                if (url_file_path == "/") url_file_path = "/index.html";
                std::string doc = doc_root + url_file_path;
                if (!boost::filesystem::exists(doc))
                    // file does not exist - send 404 error
                    asio::async_write(socket, asio::buffer( msg_404_buffer ),
                                             std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                                       std::placeholders::_2));
                else {
                    // synchronous file reading - blocking function
                    std::ifstream file(doc);
                    auto ostringstream = std::ostringstream{};
                    ostringstream << file.rdbuf();
                    auto document = ostringstream.str();
                    // send file
                    send_ok_response = "HTTP/1.1 200 OK\n\n" + document;
                    asio::async_write(socket, asio::buffer(send_ok_response),
                                             std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                                       std::placeholders::_2));
                }
            }
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
public:
    listener(asio::io_context &ioc, asio::ip::tcp::endpoint endpoint, const std::string &doc_root)
            : acceptor_(ioc), socket_(ioc), doc_root_(doc_root) {
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
            std::make_shared<session>(std::move(socket_), doc_root_)->run();
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
    std::make_shared<listener>(ioc, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), vm["port"].as<uint16_t >()}, vm["directory"].as<std::string>())->run();

    // run ioservice in every thread
    std::vector<std::thread> thread_vector;
    thread_vector.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        thread_vector.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    return 0;
}
