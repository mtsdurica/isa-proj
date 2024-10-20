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
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

namespace Utils
{

typedef enum ReturnCodes
{
    IMAPCL_SUCCESS = 0,
    IMAPCL_FAILURE,
    ARGS_MISSING_SERVER,
    ARGS_MISSING_REQUIRED,
    ARGS_MISSING_OPTION,
    ARGS_UNKNOWN_ARGUMENT,
    SOCKET_CREATING,
    SOCKET_CONNECTING,
    SERVER_BAD_HOST,
    AUTH_FILE_OPEN,
    AUTH_FILE_NONEXISTENT,
    AUTH_INVALID_CREDENTIALS,
    AUTH_MISSING_CREDENTIALS,
    OUT_DIR_NONEXISTENT,
    VALIDITY_FILE_OPEN,
    CANT_ACCESS_MAILBOX,
    SOCKET_WRITING,
    INVALID_RESPONSE,
    SSL_CONTEXT_CREATE,
    SSL_CONNECTION_CREATE,
    SSL_SET_DESCRIPTOR,
    SSL_HANDSHAKE_FAILED
} ReturnCodes;

typedef enum TypeOfFetch
{
    FETCH_ALL,
    FETCH_HEADERS,
    FETCH_NEW
} TypeOfFetch;

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
    std::string Username;
    std::string Password;

    Arguments()
        : Port("143"), Encrypted(false), CertificateFile(""), CertificateFileDirectoryPath("/etc/ssl/certs/"),
          OnlyNewMails(false), OnlyMailHeaders(false), AuthFilePath(""), MailBox("INBOX"), OutDirectoryPath(""),
          Username(""), Password("") {};
} Arguments;

/**
 * @brief Print error message to standard error
 *
 * @param returnCode Error return code
 * @param errorMessage Error message to be printed
 */
inline Utils::ReturnCodes PrintError(ReturnCodes returnCode, const std::string &errorMessage)
{
    std::cerr << "\033[1m"
              << "[" << returnCode << "] "
              << "ERROR: "
              << "\033[0m" << errorMessage << "\n";
    return returnCode;
}

/**
 * @brief Validate response recieved from the IMAP server
 *
 * @param response Response from the server
 * @param expressionString String containing validation expression
 *
 * @return False if response is valid
 */
inline bool ValidateResponse(const std::string &response, const std::string &expressionString)
{
    std::regex expression(expressionString);
    std::smatch match;
    return (!std::regex_search(response, match, expression));
}

/**
 * @brief Check command line arguments
 *
 * @param argc Number of arguments
 * @param args Array of C strings containing arguments
 * @param arguments Configuration structure where parsed arguments will be stored
 *
 * @return IMAPCL_SUCCESS if nothing failed, otherwise a positive integer
 */
inline Utils::ReturnCodes CheckArguments(int argc, char **args, Arguments &arguments)
{
    int opt;
    bool serverAddressSet = false;
    bool authFileSet = false;
    bool outDirectorySet = false;
    opterr = 0;
    while ((opt = getopt(argc, args, "a:o:p:c:C:b:Tnh")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'o':
                case 'a':
                case 'c':
                case 'C':
                case 'b':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.Port = optarg;
            break;
        case 'T':
            arguments.Encrypted = true;
            break;
        case 'c':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'o':
                case 'p':
                case 'a':
                case 'C':
                case 'b':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.CertificateFile = optarg;
            break;
        case 'C':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'o':
                case 'p':
                case 'c':
                case 'a':
                case 'b':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.CertificateFileDirectoryPath = optarg;
            break;
        case 'n':
            arguments.OnlyNewMails = true;
            break;
        case 'h':
            arguments.OnlyMailHeaders = true;
            break;
        case 'a':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'o':
                case 'p':
                case 'c':
                case 'C':
                case 'b':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.AuthFilePath = optarg;
            authFileSet = true;
            break;
        case 'b':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'o':
                case 'p':
                case 'c':
                case 'C':
                case 'a':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.MailBox = optarg;
            break;
        case 'o':
            if (optarg[0] == '-')
            {
                switch (optarg[1])
                {
                case 'a':
                case 'p':
                case 'c':
                case 'C':
                case 'b':
                case 'n':
                case 'h':
                    PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                    return Utils::ARGS_MISSING_OPTION;
                }
            }
            arguments.OutDirectoryPath = optarg;
            outDirectorySet = true;
            break;
        case '?':
            // Handling '-a' or '-o' being last argument and without its required option
            if (optopt == 'a' || optopt == 'o')
            {
                PrintError(Utils::ARGS_MISSING_OPTION, "Missing required argument option");
                return Utils::ARGS_MISSING_OPTION;
            }
            PrintError(Utils::ARGS_UNKNOWN_ARGUMENT, "Unknown argument");
            return Utils::ARGS_UNKNOWN_ARGUMENT;
        default:
            PrintError(Utils::ARGS_UNKNOWN_ARGUMENT, "Unknown argument");
            return Utils::ARGS_UNKNOWN_ARGUMENT;
        }
    }

    // TODO: parsing of server address
    for (int i = optind; i < argc; i++)
    {
        arguments.ServerAddress = args[i];
        serverAddressSet = true;
    }

    if (!serverAddressSet)
    {
        PrintError(Utils::ARGS_MISSING_SERVER, "Missing server address");
        return Utils::ARGS_MISSING_SERVER;
    }

    if (!outDirectorySet)
    {
        PrintError(Utils::ARGS_MISSING_REQUIRED, "Missing required argument");
        return Utils::ARGS_MISSING_REQUIRED;
    }

    if (!authFileSet)
    {
        PrintError(Utils::ARGS_MISSING_REQUIRED, "Missing required argument");
        return Utils::ARGS_MISSING_REQUIRED;
    }

    if (outDirectorySet)
    {
        struct stat buffer;
        if (stat(arguments.OutDirectoryPath.c_str(), &buffer) != 0)
        {
            Utils::PrintError(Utils::OUT_DIR_NONEXISTENT, "Output directory does not exist");
            return Utils::OUT_DIR_NONEXISTENT;
        }
    }

    if (authFileSet)
    {
        // Checking if auth file exists
        struct stat buffer;
        if (stat(arguments.AuthFilePath.c_str(), &buffer) != 0)
        {
            Utils::PrintError(Utils::AUTH_FILE_NONEXISTENT, "Auth file does not exist");
            return Utils::AUTH_FILE_NONEXISTENT;
        }
        // Parsing auth file
        std::ifstream authFile(arguments.AuthFilePath);
        std::string line;
        if (!authFile.is_open())
        {
            Utils::PrintError(Utils::AUTH_FILE_OPEN, "Error opening file");
            return Utils::AUTH_FILE_OPEN;
        }
        while (std::getline(authFile, line))
        {
            std::smatch matched;
            std::regex usernameRegex("username\\s=\\s(.+)");
            if (std::regex_search(line, matched, usernameRegex))
                arguments.Username = matched[1];
            std::regex passwordRegex("password\\s=\\s(.+)");
            if (std::regex_search(line, matched, passwordRegex))
                arguments.Password = matched[1];
        }
        authFile.close();
        if (arguments.Username == "")
        {
            Utils::PrintError(Utils::AUTH_MISSING_CREDENTIALS, "Missing username");
            return Utils::AUTH_MISSING_CREDENTIALS;
        }
        if (arguments.Password == "")
        {
            Utils::PrintError(Utils::AUTH_MISSING_CREDENTIALS, "Missing password");
            return Utils::AUTH_MISSING_CREDENTIALS;
        }
    }
    // Setting default port if encryption is used
    if (arguments.Encrypted)
        if (arguments.Port == "143")
            arguments.Port = "993";

    return Utils::IMAPCL_SUCCESS;
}
} // namespace Utils
