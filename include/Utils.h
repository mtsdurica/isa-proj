/**
 * @file Utils.h
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains declarations of utilities
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

#define DEBUG

namespace Utils
{

typedef enum ReturnCodes
{
    IMAPCL_SUCCESS = 0,
    IMAPCL_FAILURE,
    ARGS_MISSING_SERVER,
    ARGS_MISSING_REQUIRED,
    ARGS_UNKNOWN_ARGUMENT,
    SOCKET_CREATING,
    SOCKET_CONNECTING,
    SERVER_BAD_HOST,
    AUTH_FILE_OPEN,
    AUTH_INVALID_CREDENTIALS,
    AUTH_MISSING_CREDENTIALS,
} ReturnCodes;

typedef struct Arguments
{
    std::string ServerAddress;
    std::string Port;
    bool Encrypted;
    std::string CertificateFile;
    std::string CertificateFileDirectoryPath;
    bool OnlyNewMails;
    bool OnlyMailHeaders;
    std::string AuthFilePath;
    std::string MailBox;
    std::string OutDirectoryPath;

    Arguments()
        : Port(""), Encrypted(false), CertificateFile(""), CertificateFileDirectoryPath(""), OnlyNewMails(false),
          OnlyMailHeaders(false), AuthFilePath(""), MailBox("INBOX"), OutDirectoryPath("") {};
} Arguments;

inline void PrintError(ReturnCodes returnCode, std::string errorMessage)
{
    std::cerr << "\033[1m" << "[" << returnCode << "] " << "ERROR: " << "\033[0m" << errorMessage << "\n";
}

inline Utils::ReturnCodes CheckArguments(int argc, char **argv, Arguments &arguments)
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
            arguments.Port = optarg;
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
            return Utils::ARGS_UNKNOWN_ARGUMENT;
            break;
        default:
            std::cerr << "UNKNOWN ARGS!" << "\n";
            return Utils::ARGS_UNKNOWN_ARGUMENT;
            break;
        }
    }

    // TODO: parsing of server address
    for (int i = optind; i < argc; i++)
    {
        arguments.ServerAddress = argv[i];
        serverAddressSet = true;
    }

    if (helpFlag)
    {
        // print help
        std::cerr << "deez" << "\n";
        return Utils::IMAPCL_SUCCESS;
    }

    if (!serverAddressSet)
    {
        PrintError(Utils::ARGS_MISSING_SERVER, "Missing server address");
        return Utils::ARGS_MISSING_SERVER;
    }

    if (!authFileSet)
    {
        PrintError(Utils::ARGS_MISSING_REQUIRED, "Missing required arguments");
        return Utils::ARGS_MISSING_REQUIRED;
    }

    if (!outDirectorySet)
    {
        PrintError(Utils::ARGS_MISSING_REQUIRED, "Missing required arguments");
        return Utils::ARGS_MISSING_REQUIRED;
    }

    return Utils::IMAPCL_SUCCESS;
}
} // namespace Utils
