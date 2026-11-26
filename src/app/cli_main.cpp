#include "core/version.h"

#include <cstring>
#include <iostream>

namespace {

bool hasArgument(int argc, char **argv, const char *name)
{
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], name) == 0) {
            return true;
        }
    }
    return false;
}

void printUsage()
{
    std::cout << tmog::productName().toStdString() << "\n\n"
              << "Usage: tmog-cli [options]\n\n"
              << "Options:\n"
              << "  --version    Print the product name and version, then exit\n"
              << "  --help       Show this help\n";
}

} // namespace

int main(int argc, char **argv)
{
    if (hasArgument(argc, argv, "--version")) {
        std::cout << tmog::versionLine().toStdString() << "\n";
        return 0;
    }

    if (hasArgument(argc, argv, "--help")) {
        printUsage();
        return 0;
    }

    printUsage();
    return 2;
}
