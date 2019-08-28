#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include "http.h"
#include "casher.h"
#include <boost/algorithm/string/trim.hpp>

namespace http {
    request::request(asio::streambuf &buff) {
        std::vector<std::string> list;
        auto input_string = std::string(std::istreambuf_iterator<char>(&buff), std::istreambuf_iterator<char>());
        boost::split(list, input_string, boost::is_any_of(" "));
        method = list[0];
        URI = list[1];
        version = list[2];
        boost::algorithm::trim(version);
    }

    void request::write_response(asio::streambuf &buff, const std::string &doc_root, casher &cash) {
        std::ostream out(&buff);
        if (method == "GET") {
            // currently supports only GET methods
            std::string doc = doc_root + (URI == "/" ? "/index.html" : URI);
            // look for index.html if no file specified
            if (boost::filesystem::exists(doc)) {
                out << version << " " << ok << "\n" << cash.get_response_file(doc) << std::endl;
            } else
                out << version << " " << not_found << "\n" << not_found_body << std::endl;
        } else
            out << version << " " << not_implemented << "\n" << not_implemented_body << std::endl;
    }
}
