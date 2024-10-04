#pragma once

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>

namespace Utils
{

typedef enum ReturnCodes
{
    ARGS_MISSING_SERVER = 2,
    ARGS_MISSING_REQUIRED,
    ARGS_UNKNOWN_ARGUMENT
} ReturnCodes;

typedef struct Arguments
{
    std::string ServerAddress;
    int Port;
    bool Encrypted;
    std::string CertificateFile;
    std::string CertificateFileDirectoryPath;
    bool OnlyNewMails;
    bool OnlyMailHeaders;
    std::string AuthFilePath;
    std::string MailBox;
    std::string OutDirectoryPath;

    Arguments()
        : Port(-1), Encrypted(false), CertificateFile(""), CertificateFileDirectoryPath(""), OnlyNewMails(false),
          OnlyMailHeaders(false), AuthFilePath(""), MailBox("INBOX"), OutDirectoryPath("") {};
} Arguments;

inline void PrintError(ReturnCodes returnCode, std::string errorMessage)
{
    std::cerr << "[" << returnCode << "] " << "ERROR: " << errorMessage << "\n";
}

inline int CheckArguments(int argc, char **argv, Arguments &arguments)
{
    int opt;
    bool helpFlag = false;
    bool serverAddressSet = false;
    bool authFileSet = false;
    bool outDirectorySet = false;
    opterr = 0;
    struct option longOptions[] = {{"help", no_argument, 0, 0},
                                   {0, required_argument, 0, 'p'},
                                   {0, no_argument, 0, 'T'},
                                   {0, required_argument, 0, 'c'},
                                   {0, required_argument, 0, 'C'},
                                   {0, no_argument, 0, 'n'},
                                   {0, no_argument, 0, 'h'},
                                   {0, required_argument, 0, 'a'},
                                   {0, required_argument, 0, 'b'},
                                   {0, required_argument, 0, 'o'},
                                   {0, 0, 0, 0}};
    int optionIndex = 0;
    while ((opt = getopt_long(argc, argv, "p:c:C:a:b:o:Tnh0", longOptions, &optionIndex)) != -1)
    {
        switch (opt)
        {
        case 0: {
            std::string argName = longOptions[optionIndex].name;
            if (argName == "help")
            {
                std::cerr << "Option " << longOptions[optionIndex].name << "\n";
                helpFlag = true;
            }
            break;
        }
        case 'p':
            arguments.Port = std::stoi(optarg);
            break;
        case 'T':
            arguments.Encrypted = true;
            break;
        case 'c':
            arguments.CertificateFile = optarg;
            break;
        case 'C':
            arguments.CertificateFileDirectoryPath = optarg;
            break;
        case 'n':
            arguments.OnlyNewMails = true;
            break;
        case 'h':
            arguments.OnlyMailHeaders = true;
            break;
        case 'a':
            arguments.AuthFilePath = optarg;
            authFileSet = true;
            break;
        case 'b':
            arguments.MailBox = optarg;
            break;
        case 'o':
            arguments.OutDirectoryPath = optarg;
            outDirectorySet = true;
            break;
        case '?':
            std::cerr << "DEEZ" << opt << "\n";
            return ARGS_UNKNOWN_ARGUMENT;
            break;
        default:
            std::cerr << "UNKNOWN ARGS!" << "\n";
            return ARGS_UNKNOWN_ARGUMENT;
            break;
        }
    }

    // TODO: parsing of server address
    for (int i = optind; i < argc; i++)
    {
        std::cerr << "ARG " << argv[i] << "\n";
        serverAddressSet = true;
    }

    if (helpFlag)
    {
        // print help
        std::cerr << "deez" << "\n";
        return EXIT_SUCCESS;
    }

    if (!serverAddressSet)
    {
        PrintError(ARGS_MISSING_SERVER, "MISSING SERVER ADDRESS");
        return ARGS_MISSING_SERVER;
    }

    if (!authFileSet)
    {
        PrintError(ARGS_MISSING_REQUIRED, "MISSING REQUIRED ARG");
        return ARGS_MISSING_REQUIRED;
    }

    if (!outDirectorySet)
    {
        PrintError(ARGS_MISSING_REQUIRED, "MISSING REQUIRED ARG");
        return ARGS_MISSING_REQUIRED;
    }

    return EXIT_SUCCESS;
}
} // namespace Utils
