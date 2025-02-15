#include "cli.h"

#include <iosfwd>
#include <iostream>

int main(const int argc, char *argv[]) {
    // Disable stdio synchronization to speed up stdin/stdout
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    parse_args(argc, argv);
    return 0;
}
