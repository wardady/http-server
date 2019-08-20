#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "http.h"

namespace http {
    request::request(asio::streambuf &buff) {
        buff.commit(buff.size());
        std::istream is(&buff);
        std::string req;
        is >> req;

        std::vector<std::string> list;
        boost::split(list, req, boost::is_any_of(" "));
        method = list[0];
        URI = list[1];
        version = list[2];
    }
}
