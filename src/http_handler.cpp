//
// Created by hermann on 20.05.19.
//
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <iostream>

//#include "../inc/http_handler.hpp"
class Session {

    Session(boost::asio::ip::tcp::socket &socket) : boost::asio::ip::tcp::socket

    socket{ socket }{

    }
};

// Return a reasonable mime type based on the extension of a file.
std::string mime_type(std::string_view path) {
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        return path.substr(pos);
    }();
    if (ext == ".htm") return "text/html";
    if (ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".txt") return "text/plain";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".swf") return "application/x-shockwave-flash";
    if (ext == ".flv") return "video/x-flv";
    if (ext == ".png") return "image/png";
    if (ext == ".jpe") return "image/jpeg";
    if (ext == ".jpeg") return "image/jpeg";
    if (ext == ".jpg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".bmp") return "image/bmp";
    if (ext == ".ico") return "image/vnd.microsoft.icon";
    if (ext == ".tiff") return "image/tiff";
    if (ext == ".tif") return "image/tiff";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".svgz") return "image/svg+xml";
    return "application/text";
}


