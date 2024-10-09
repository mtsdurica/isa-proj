/**
 * @file Communication.h
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains declaration of Communication class
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/Utils.h"

class Communication
{
  protected:
    int SocketDescriptor;    // Socket descriptor
    struct addrinfo *Server; // Structure containing host information
    std::string Username;
    std::string Password;
    std::string Buffer;

  public:
    Communication();
    ~Communication();
    /**
     * @brief Get address info about host
     *
     * @param serverAddress Server hostname
     * @param port Server port
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise SERVER_BAD_HOST
     */
    Utils::ReturnCodes GetHostAddressInfo(const std::string &serverAddress, const std::string &port);
    /**
     * @brief Create communication socket
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise SOCKET_CREATING
     */
    Utils::ReturnCodes CreateSocket();
    /**
     * @brief Connect to socket
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise SOCKET_CONNECTING
     */
    Utils::ReturnCodes Connect();
    /**
     * @brief Authenticate user on the server
     *
     * @return IMAPCL_SUCCESS if nothing failed
     */
    Utils::ReturnCodes Authenticate(const std::string &authFilePath);
    /**
     * @brief Logout user from session
     *
     * @return IMAPCL_SUCCESS if nothing failed
     */
    Utils::ReturnCodes Logout();
    /**
     * @brief Get socket descriptor
     *
     * @return Socket descriptor
     */
    int GetSocketDescriptor();
};
