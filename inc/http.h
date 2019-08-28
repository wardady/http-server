#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <boost/asio/streambuf.hpp>
#include <string>
#include "casher.h"

namespace asio = boost::asio;

namespace http {
    constexpr char ok[] = "200 OK\n";
    constexpr char not_found[] = "404 NOT FOUND\n";
    constexpr char not_found_body[] = "<html><body><h1>404 Not Found</h1></body></html>";
    constexpr char not_implemented[] = "501 NOT IMPLEMENTED\n";
    constexpr char not_implemented_body[] = "<html><body><h1>501 Not Implemented</h1></body></html>";

    class request {
        std::string method;
        std::string URI;
        std::string version;
    public:
        explicit request(asio::streambuf &buff);
        void write_response(asio::streambuf &buff, const std::string &doc_root,casher &cash);
    };
}

#endif //SERVER_HTTP_H
