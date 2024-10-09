/**
 * @file imapcl.cpp
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Program entry point
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/Communication.h"
#include "../include/Utils.h"

int main(int argc, char **argv)
{
    std::string buffer("", 1024);
    Utils::Arguments arguments;
    if (Utils::CheckArguments(argc, argv, arguments))
        return Utils::IMAPCL_FAILURE;
#ifdef DEBUG
    std::cerr << "---------------DEBUG---------------" << "\n";
    std::cerr << arguments.OutDirectoryPath << "\n";
    std::cerr << arguments.AuthFilePath << "\n";
    std::cerr << arguments.ServerAddress << "\n";
    std::cerr << "-----------------------------------" << "\n";
#endif
    Communication communication;
    if (communication.GetHostAddressInfo(arguments.ServerAddress, arguments.Port))
        return Utils::SERVER_BAD_HOST;
    if (communication.CreateSocket())
        return Utils::SOCKET_CREATING;
    if (communication.Connect())
        return Utils::SOCKET_CONNECTING;
    if (communication.Authenticate(arguments.AuthFilePath))
        return Utils::AUTH_FILE_OPEN;
    buffer = "A1 logout\n";
    send(communication.GetSocketDescriptor(), buffer.c_str(), buffer.length(), 0);
    buffer = std::string("", 1024);
    while (true)
    {
        int received = recv(communication.GetSocketDescriptor(), buffer.data(), buffer.length(), 0);
        if (received != 0)
        {
            std::cerr << "RECV: " << std::to_string(received) << "\n";
            std::cout << buffer.substr(0, received) << "\n";
            break;
        }
    }
    return Utils::IMAPCL_SUCCESS;
}
