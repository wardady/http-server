#include "cli.h"
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

cli::cli(int argc, char **argv) {
    // define options to be shown on --help
    po::options_description basic_options("Options");
    basic_options.add_options()
            ("help", "print help message")
            ("port,p", po::value<uint16_t>(), "specify port number")
            ("threads,t", po::value<int>()->default_value(2), "specify number of threads to use")
            ("no-cache", "do not cache web-pages");

    // define hidden options
    po::options_description hidden_options("Hidden options");
    hidden_options.add_options()
            ("directory,d", po::value<std::string>(), "directory to serve from");

    po::options_description options;
    options.add(basic_options).add(hidden_options);

    // define positional options
    po::positional_options_description positional;
    positional.add("directory", 1);

    po::variables_map vm;
    auto parsed = po::command_line_parser(argc, argv).options(options).positional(positional).run();
    po::store(parsed, vm);

    // show help message
    if (vm.count("help")) {
        std::cout << basic_options << std::endl;
        exit(0);
    }

    // port is not specified
    if (!vm.count("port")) {
        throw std::invalid_argument("Port to use is not specified\n");
    }

    // directory is not specified
    if (!vm.count("directory")) {
        throw std::invalid_argument("Directory to serve from is not specified\n");
    }
    threads = std::max(1, vm["threads"].as<int>());
    port = vm["port"].as<unsigned short>();
    no_cache = vm.count("no-cache");
    directory = vm["directory"].as<std::string>();
}

int cli::get_threads() {
    return threads;
}

unsigned short cli::get_port() {
    return port;
}

std::string &cli::get_directory() {
    return std::ref(directory);
}

bool cli::get_no_cache() {
    return no_cache;
}

