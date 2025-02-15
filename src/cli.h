#ifndef CLI_H
#define CLI_H
#include <cstdint>
#include <vector>

void parse_args(int argc, char *argv[]);

std::vector<uint8_t> read_stdin();

#endif //CLI_H
