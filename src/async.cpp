#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <QString>
#include <QVector>
#include <QStringList>

class session : public std::enable_shared_from_this<session> {
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf buff;
    const std::string &doc_root;
public:
    explicit session(boost::asio::ip::tcp::socket socket, const std::string &doc_root)
            : socket(std::move(socket)), doc_root(doc_root) {}

    void run() { on_connect(); }

    // read client's http request
    void on_connect() {
        boost::asio::async_read(socket, buff, boost::asio::transfer_at_least(1),
                                std::bind(&session::on_read, shared_from_this(), std::placeholders::_1,
                                          std::placeholders::_2));
    }

    // callback for reading client's http request
    void on_read(boost::system::error_code ec, std::size_t s) {
        if (!ec) {
            // split request by space
            auto list = QString::fromStdString(
                    std::string((std::istreambuf_iterator<char>(&buff)), std::istreambuf_iterator<char>())).split(" ");

            if (list[0] != "GET")
                // only GET requests are supported
                boost::asio::async_write(socket, boost::asio::buffer("HTTP/1.1 501 NOT IMPLEMENTED"),
                                         std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                                   std::placeholders::_2));
            else {
                // if no file requested return index.html
                if (list[1] == "/") list[1] = "/index.html";
                std::string doc = doc_root + list[1].toStdString();
                if (!boost::filesystem::exists(doc))
                    // file does not exist - send 404 error
                    boost::asio::async_write(socket, boost::asio::buffer("HTTP/1.1 404 NOT FOUND"),
                                             std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                                                       std::placeholders::_2));
                else {
                    // synchronous file reading - blocking function
                    std::ifstream file(doc);
                    auto ostringstream = std::ostringstream{};
                    ostringstream << file.rdbuf();
                    auto document = ostringstream.str();
                    // send file
                    boost::asio::async_write(socket, boost::asio::buffer("HTTP/1.1 200 OK\n\n" + document),
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
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        else
            std::cerr << "Writing reply error: " << ec.message() << std::endl;
    }
};

class listener : public std::enable_shared_from_this<listener> {
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    const std::string &doc_root_;
public:
    listener(boost::asio::io_context &ioc, boost::asio::ip::tcp::endpoint endpoint, const std::string &doc_root)
            : acceptor_(ioc), socket_(ioc), doc_root_(doc_root) {
        boost::system::error_code ec;

        // initialize acceptor by endpoint
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.bind(endpoint, ec);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);

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
    // check command line arguments number
    if (argc != 5) {
        std::cerr << "Usage: async <address> <port> <doc_root> <threads>" << std::endl;
        return 1;
    }

    // process command line arguments
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::stoi(argv[2]));
    std::string doc_root = argv[3];
    auto const threads = std::max<int>(1, std::stoi(argv[4]));

    // create ioservice shared between all threads
    boost::asio::io_service ioc{threads};
    // initialize and run listener
    std::make_shared<listener>(ioc, boost::asio::ip::tcp::endpoint{address, port}, doc_root)->run();

    // run ioservice in every thread
    std::vector<std::thread> thread_vector;
    thread_vector.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        thread_vector.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    return 0;
}
