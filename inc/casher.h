#ifndef SERVER_CASHER_H
#define SERVER_CASHER_H

#include "tbb/concurrent_hash_map.h"
#include <string>

class casher {
public:
    casher(bool toHash);

    std::string get_response_file(const std::string& filename);
private:
    tbb::concurrent_hash_map<std::string, std::string> buffered_files;
    const bool hashing;
};

#endif //SERVER_CASHER_H
