/**
 * @file Session.h
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains declaration of Session class
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
class Session
{
  protected:
    int SocketDescriptor;    // Socket descriptor
    struct addrinfo *Server; // Structure containing host information
    std::string ServerHostname;
    std::string Port;
    std::string Username;
    std::string Password;
    std::string Buffer;
    std::string FullResponse;
    std::string OutDirectoryPath;
    std::string MailBox;
    int CurrentTagNumber;
    Utils::ReturnCodes ReturnCode;
    /**
     * @brief Validate UIDValidity of a mailbox.
     * If validity file does not exist, it is created and the UIDValidity is written to it. If it exists and UIDValidity
     * does not match, MailBoxValidated variable is set to `true` and the mailbox will be redownloaded.
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise VALIDITY_FILE_OPEN
     */
    Utils::ReturnCodes ValidateMailbox(Utils::TypeOfFetch typeOfFetch);
    /**
     * @brief Select mailbox from which to fetch mail.
     *
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes SelectMailbox(Utils::TypeOfFetch typeOfFetch);
    std::vector<std::string> SearchMailbox(const std::string &searchKey);
    std::vector<std::string> SearchLocalMailDirectory();

  public:
    Session(const std::string &serverHostname, const std::string &port, const std::string &username,
            const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox);
    ~Session();
    /**
     * @brief Get address info about host
     *
     * @param serverAddress Server hostname
     * @param port Server port
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise SERVER_BAD_HOST
     */
    Utils::ReturnCodes GetHostAddressInfo();
    /**
     * @brief Create communication socket
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise SOCKET_CREATING
     */
    Utils::ReturnCodes CreateSocket();
    /**
     *
     */
    void SendMessage(const std::string &message);
    /**
     *
     */
    void ReceiveUntaggedResponse();
    /**
     *
     */
    void ReceiveTaggedResponse();
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
    Utils::ReturnCodes Authenticate();
    /**
     * @brief Fetch all mail from a mailbox.
     *
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes FetchMail(const bool newMailOnly);
    Utils::ReturnCodes FetchHeaders(const bool newMailOnly);
    /**
     * @brief Logout user from session
     *
     * @return IMAPCL_SUCCESS if nothing failed
     */
    Utils::ReturnCodes Logout();
};
