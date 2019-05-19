#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>

typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

void write_handler(socket_ptr socket, const boost::system::error_code &ec, std::size_t bytes_transferred) {
    if (!ec)
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    else
        // write error message
        std::cerr << "Writing error: " << ec.message() << std::endl;
}

void accept_handler(boost::asio::io_service &ioservice, socket_ptr socket, acceptor_ptr acceptor, std::string &body,
                    const boost::system::error_code &ec) {
    if (!ec) {
        // send response; try to shut down the socket after
        boost::asio::async_write(*socket, boost::asio::buffer("HTTP/1.1 200 OK\n\n" + body),
                                 boost::bind(write_handler, socket, _1, _2));
        // create new socket and wait for connection
        socket_ptr socket(new boost::asio::ip::tcp::socket{ioservice});
        acceptor->async_accept(*socket,
                               boost::bind(accept_handler, std::ref(ioservice), socket, acceptor, std::ref(body), _1));
    } else
        // write error message
        std::cerr << "Connection error: " << ec.message() << std::endl;
}

// starts server on single thread
void serve(boost::asio::io_service &ioservice, acceptor_ptr acceptor, std::string &body) {
    // create first socket on the thread
    socket_ptr socket(new boost::asio::ip::tcp::socket{ioservice});
    // accept connection to socket; call accept_handler when connection establish
    acceptor->async_accept(*socket,
                           boost::bind(accept_handler, std::ref(ioservice), socket, acceptor, std::ref(body), _1));
    // wait until all asynchronous operations end
    ioservice.run();
}

int main(int argc, char *argv[], char *envp[]) {
    // usage error
    if (argc != 5) {
        std::cerr << "Usage: async <address> <port> <doc> <threads>\n" << std::endl;
        return 1;
    }

    // parse program arguments
    auto address = boost::asio::ip::address::from_string(argv[1]);
    unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
    int threads = std::max<int>(1, std::atoi(argv[4]));

    // try to open file
    std::ifstream file(argv[3]);
    if (file.fail()) {
        std::cerr << "Failed to open " << argv[3] << std::endl;
        return 1;
    }

    // read file
    auto ss = std::ostringstream{};
    ss << file.rdbuf();
    auto body = ss.str();

    // initialize shared asio objects
    boost::asio::io_service ioservice;
    boost::asio::ip::tcp::endpoint ep{address, port};
    acceptor_ptr acceptor(new boost::asio::ip::tcp::acceptor{ioservice, ep});

    // create vector of one less threads because of the main one
    std::vector<std::thread> threads_vector;
    threads_vector.reserve(threads - 1);

    // start server
    acceptor->listen();
    std::cout << "Serving HTTP on " << address.to_string() << " port " << port << std::endl;
    for (auto i = threads - 1; i > 0; --i) {
        threads_vector.emplace_back(serve, std::ref(ioservice), acceptor, std::ref(body));
    }
    serve(std::ref(ioservice), acceptor, std::ref(body));

    return 0;
}
