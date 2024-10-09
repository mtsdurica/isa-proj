/**
 * @file Communication.cpp
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains definitions of Communication class methods
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdlib>
#include <fstream>
#include <regex>
#include <string>
#include <sys/socket.h>

#include "../include/Communication.h"

Communication::Communication() : SocketDescriptor(-1), Server(nullptr), Username(""), Password("")
{
    this->Buffer = std::string("", 1024);
}

Communication::~Communication()
{
    if (this->Server != nullptr)
        freeaddrinfo(this->Server);
    if (this->SocketDescriptor > 0)
    {
        shutdown(this->SocketDescriptor, SHUT_RDWR);
        close(this->SocketDescriptor);
    }
}

Utils::ReturnCodes Communication::GetHostAddressInfo(const std::string &serverAddress, const std::string &port)
{
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(serverAddress.c_str(), port.c_str(), &hints, &(this->Server)) != 0)
    {
        Utils::PrintError(Utils::SERVER_BAD_HOST, "Bad host");
        return Utils::SERVER_BAD_HOST;
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::CreateSocket()
{
    if ((this->SocketDescriptor =
             socket(this->Server->ai_family, this->Server->ai_socktype, this->Server->ai_protocol)) <= 0)
    {
        Utils::PrintError(Utils::SOCKET_CREATING, "Error creating socket");
        return Utils::SOCKET_CREATING;
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::Connect()
{
    if (connect(this->SocketDescriptor, Server->ai_addr, Server->ai_addrlen) == -1)
    {
        Utils::PrintError(Utils::SOCKET_CONNECTING, "Connecting to socket failed");
        return Utils::SOCKET_CONNECTING;
    }
    while (true)
    {
        int received = recv(this->SocketDescriptor, this->Buffer.data(), this->Buffer.length(), 0);
        if (received != 0)
        {
            std::cerr << "RECV: " << std::to_string(received) << "\n";
            std::cout << this->Buffer.substr(0, received);
            break;
        }
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::Authenticate(const std::string &authFilePath)
{
    std::ifstream authFile(authFilePath);
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
            this->Username = matched[1];
        std::regex passwordRegex("password\\s=\\s(.+)");
        if (std::regex_search(line, matched, passwordRegex))
            this->Password = matched[1];
    }
    authFile.close();
    if (this->Username == "")
    {
        Utils::PrintError(Utils::AUTH_MISSING_CREDENTIALS, "Missing username");
        return Utils::AUTH_MISSING_CREDENTIALS;
    }
    if (this->Password == "")
    {
        Utils::PrintError(Utils::AUTH_MISSING_CREDENTIALS, "Missing password");
        return Utils::AUTH_MISSING_CREDENTIALS;
    }
    this->Buffer = "A1 login";
    this->Buffer += " " + this->Username + " " + this->Password + "\n";
    send(this->SocketDescriptor, this->Buffer.c_str(), this->Buffer.length(), 0);
    this->Buffer = std::string("", 1024);
    while (true)
    {
        int received = recv(this->SocketDescriptor, this->Buffer.data(), this->Buffer.length(), 0);
        if (received != 0)
        {
            std::cerr << "RECV: " << std::to_string(received) << "\n";
            std::cout << this->Buffer.substr(0, received);
            break;
        }
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::Logout()
{
    this->Buffer = "A1 logout\n";
    send(this->SocketDescriptor, this->Buffer.c_str(), this->Buffer.length(), 0);
    this->Buffer = std::string("", 1024);
    while (true)
    {
        int received = recv(this->SocketDescriptor, this->Buffer.data(), this->Buffer.length(), 0);
        if (received != 0)
        {
            std::cerr << "RECV: " << std::to_string(received) << "\n";
            std::cout << this->Buffer.substr(0, received);
            break;
        }
    }
    return Utils::IMAPCL_SUCCESS;
}

int Communication::GetSocketDescriptor()
{
    return this->SocketDescriptor;
}
