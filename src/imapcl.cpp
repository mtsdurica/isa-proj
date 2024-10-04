#include <cstdlib>
#include <iostream>

#include "../include/Utils.h"

int main(int argc, char **argv)
{
    std::string ad = "";
    Utils::Arguments arguments;
    if (Utils::CheckArguments(argc, argv, arguments))
        return EXIT_FAILURE;
    std::cerr << arguments.OutDirectoryPath << "\n";
    std::cerr << "PORT: " << arguments.Port << "\n";
    std::cerr << arguments.OnlyNewMails << "\n";
    std::cerr << ad << "\n";
    return EXIT_SUCCESS;
}
