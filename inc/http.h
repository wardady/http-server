#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <boost/asio/streambuf.hpp>
#include <string>

namespace asio = boost::asio;

namespace http {
    class request {
    public:
        explicit request(asio::streambuf &buff);
        std::string method;
        std::string URI;
        std::string version;
    };
}

#endif //SERVER_HTTP_H
