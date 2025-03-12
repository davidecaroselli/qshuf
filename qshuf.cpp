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
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options] <input_file>" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -t, --threads <num>  Number of threads to use (default: 1)" << std::endl;
            std::cout << "  -h, --help           Display this help message" << std::endl;
            return 0;
        } else {
            if (input_file != nullptr) {
                std::cerr << "Error: multiple input files specified" << std::endl;
                return 1;
            }

            input_file = argv[i];
            i++;
        }
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
