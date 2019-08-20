#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include "http.h"

namespace http {
    request::request(asio::streambuf &buff) {
        std::vector<std::string> list;
        auto input_string = std::string(std::istreambuf_iterator<char>(&buff), std::istreambuf_iterator<char>());
        boost::split(list, input_string, boost::is_any_of(" "));
        method = list[0];
        URI = list[1];
        version = list[2];
    }

    void request::write_response(asio::streambuf &buff, const std::string &doc_root) {
        std::ostream out(&buff);
        if (method == "GET") {
            // currently supports only GET methods
            std::string doc = doc_root + (URI == "/" ? "/index.html" : "/");
            // look for index.html if no file specified
            if (boost::filesystem::exists(doc)) {
                std::ifstream file(doc);
                out << version << " " << ok << "\n" << file.rdbuf() << std::endl;
            } else
                out << version << " " << not_found << "\n" << not_found_body << std::endl;
        } else
            out << version << " " << not_implemented << "\n" << not_implemented_body << std::endl;
    }
}
