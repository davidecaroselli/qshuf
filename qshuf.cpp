#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <cstring>
#include <algorithm>
#include <random>

#define CLI_SUCCESS 0
#define CLI_INVALID_OPTION 2

#define QSHUF_VERSION "0.0.1"

struct mmap_buffer {
    char *start;
    size_t length;

    mmap_buffer(char *start, size_t length) : start(start), length(length) {
    }
};

void collect_lines(const char *start, const char *end, std::vector<mmap_buffer> *output) {
    output->clear();

    auto begin = const_cast<char *>(start);
    for (const char *ptr = start; ptr < end; ++ptr) {
        if (*ptr == '\n') {
            output->emplace_back(begin, ptr - begin);
            begin = const_cast<char *>(ptr + 1); // Move to next line
        }
    }
}

std::vector<mmap_buffer> collect_lines_multithreaded(const mmap_buffer &data, int num_threads) {
    std::vector<std::thread> threads;
    std::vector<std::vector<mmap_buffer>> outputs(num_threads);

    const char *eof = data.start + data.length;

    const char *chunk_start = data.start;
    for (int i = 0; i < num_threads; ++i) {
        const char *chunk_end;

        if (i == num_threads - 1) {
            chunk_end = eof;
        } else {
            chunk_end = data.start + (i + 1) * (data.length / num_threads);

            // Adjust end to be after the next newline (including it)
            while (chunk_end < eof && *chunk_end != '\n') {
                ++chunk_end;
            }
            if (chunk_end < eof) {
                ++chunk_end;
            }
        }

        threads.emplace_back(collect_lines, chunk_start, chunk_end, &outputs[i]);
        chunk_start = chunk_end;
    }

    for (auto &t: threads) {
        t.join();
    }

    size_t total_lines = 0;
    for (const auto &output: outputs) {
        total_lines += output.size();
    }

    std::vector<mmap_buffer> all_lines;
    all_lines.reserve(total_lines);

    for (const auto &output: outputs)
        all_lines.insert(all_lines.end(), output.begin(), output.end());

    return all_lines;
}

int open_file(const std::string &file_path) {
    const int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    return fd;
}

mmap_buffer mmap_file(const int fd) {
    struct stat st{};
    if (fstat(fd, &st) == -1) {
        perror("Error getting file size");
        close(fd);
        exit(1);
    }
    size_t file_size = st.st_size;

    auto data = static_cast<char *>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (data == MAP_FAILED) {
        perror("Error memory mapping file");
        close(fd);
        exit(1);
    }

    return {data, file_size};
}

int main(int argc, char *argv[]) {
    // Parse args
    int num_threads = 1;
    char *input_file = nullptr;

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) {
            num_threads = std::stoi(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            std::cout << "qshuf " << QSHUF_VERSION << std::endl;
            std::cout << "Copyright (c) 2025 Davide Caroselli" << std::endl;
            std::cout << "License MIT: <https://opensource.org/license/MIT>" << std::endl;
            std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
            std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
            std::cout << std::endl;
            std::cout << "Written by Davide Caroselli." << std::endl;
            return CLI_SUCCESS;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: qshuf [OPTIONS] <input_file>" << std::endl;
            std::cout << "Efficiently shuffles very large text files using" << std::endl;
            std::cout << "memory mapping, minimizing RAM usage." << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -t, --threads <num>  number of threads to use (default: 1)" << std::endl;
            std::cout << "  -v, --version        output version information and exit" << std::endl;
            std::cout << "  -h, --help           display this help message" << std::endl;
            return CLI_SUCCESS;
        } else if (argv[i][0] == '-') {
            std::cerr << "qshuf: invalid option -- '" << argv[i] << "'" << std::endl;
            std::cerr << "Try 'qshuf --help' for more information." << std::endl;
            return CLI_INVALID_OPTION;
        } else {
            if (input_file != nullptr) {
                std::cerr << "qshuf: extra operand '" << argv[i] << "'" << std::endl;
                std::cerr << "Try 'qshuf --help' for more information." << std::endl;
                return CLI_INVALID_OPTION;
            }

            input_file = argv[i];
            i++;
        }
    }

    if (input_file == nullptr) {
        std::cerr << "qshuf: missing operand" << std::endl;
        std::cerr << "Try 'qshuf --help' for more information." << std::endl;
        return CLI_INVALID_OPTION;
    }

    // Open and memory map file
    const int fd = open_file(input_file);
    mmap_buffer data = mmap_file(fd);

    // Collect lines
    std::vector<mmap_buffer> lines = collect_lines_multithreaded(data, num_threads);

    // Shuffle lines
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(lines.begin(), lines.end(), rng);

    // Print lines
    for (const auto &line: lines) {
        std::cout.write(line.start, static_cast<std::streamsize>(line.length));
        std::cout << '\n';
    }

    // Cleanup
    munmap(data.start, data.length);
    close(fd);

    return 0;
}
