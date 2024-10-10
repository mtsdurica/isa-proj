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
#include <regex>
#include <string>
#include <sys/socket.h>

#include "../include/Communication.h"

Communication::Communication(const std::string &username, const std::string &password)
    : SocketDescriptor(-1), Server(nullptr), Username(username), Password(password), Buffer(std::string("", 1024)),
      FullResponse(""), CurrentTagNumber(1)
{
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

void Communication::ReceiveResponse()
{
    while (true)
    {
        unsigned long int received = recv(this->SocketDescriptor, this->Buffer.data(), BUFFER_SIZE, 0);
        if (received != 0)
        {
            std::cerr << "RECV: " << std::to_string(received) << "\n";
            this->FullResponse += this->Buffer.substr(0, received);
            if (received < BUFFER_SIZE)
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
}

Utils::ReturnCodes Communication::Connect()
{
    if (connect(this->SocketDescriptor, Server->ai_addr, Server->ai_addrlen) == -1)
    {
        Utils::PrintError(Utils::SOCKET_CONNECTING, "Connecting to socket failed");
        return Utils::SOCKET_CONNECTING;
    }
    this->ReceiveResponse();
    // TODO: FIX THIS SHIT
    std::regex responseRegex(".*");
    std::cerr << std::regex_match(this->FullResponse, responseRegex) << "\n";
    std::cerr << this->FullResponse;
    this->FullResponse = "";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::Authenticate()
{
    this->Buffer = "A" + std::to_string(this->CurrentTagNumber) + " LOGIN";
    this->Buffer += " " + this->Username + " " + this->Password + "\n";
    send(this->SocketDescriptor, this->Buffer.c_str(), this->Buffer.length(), 0);
    this->Buffer = std::string("", BUFFER_SIZE);
    this->ReceiveResponse();
    std::regex responseRegex("\\*\\sOK\\s.*");
    if (std::regex_match(this->FullResponse.begin(), this->FullResponse.end(), responseRegex))
    {
        std::cerr << this->FullResponse;
    }
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Communication::Logout()
{
    this->Buffer = "A" + std::to_string(this->CurrentTagNumber) + " LOGOUT\n";
    send(this->SocketDescriptor, this->Buffer.c_str(), this->Buffer.length(), 0);
    this->Buffer = std::string("", BUFFER_SIZE);
    this->ReceiveResponse();
    std::regex responseRegex("\\*\\sOK\\s.*");
    if (std::regex_match(this->FullResponse.begin(), this->FullResponse.end(), responseRegex))
    {
        std::cerr << this->FullResponse;
    }
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

int Communication::GetSocketDescriptor()
{
    return this->SocketDescriptor;
}
