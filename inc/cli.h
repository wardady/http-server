#ifndef SERVER_CLI_H
#define SERVER_CLI_H

#include <string>

class cli {
public:
    cli(int argc, char *argv[]);
    int get_threads();
    unsigned short get_port();
    std::string& get_directory();
    bool get_no_cache();
private:
    int threads;
    unsigned short port;
    std::string directory;
    bool no_cache;
};

#endif //SERVER_CLI_H
