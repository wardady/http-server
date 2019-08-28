#include "casher.h"
#include <fstream>
#include <sstream>

casher::casher(bool toHash) : hashing{toHash}, buffered_files{} {}

std::string casher::get_response_file(const std::string& filename) {
    std::ostringstream reader;
    if (!hashing) {
        std::ifstream file(filename);
        reader << file.rdbuf();
        return reader.str();
    } else {
        decltype(buffered_files)::accessor access;
        if (buffered_files.insert(access, filename)) {
            std::ifstream file(filename);
            reader << file.rdbuf();
            access->second = reader.str();
        }
        return access->second;
    }
}
