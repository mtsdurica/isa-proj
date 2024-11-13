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
     * does not match, MailBoxValidated variable is set to `true`, local mail from current session's server will be
     * deleted and the remote mailbox will be redownloaded.
     *
     * @return IMAPCL_SUCCESS if nothing failed, otherwise VALIDITY_FILE_OPEN
     */
    Utils::ReturnCodes ValidateMailbox();
    /**
     * @brief Select mailbox from which to fetch mail.
     *
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes SelectMailbox();
    /**
     * @brief Search mailbox for mail UIDs
     *
     * @param searchKey What messages to be received
     * @return std::tuple<std::vector<std::string>, Utils::ReturnCodes> Vector containing remote mail UIDs and
     * IMAPCL_SUCCESS if nothing failed, SOCKET_WRITING if sending a request to the server failed
     */
    std::tuple<std::vector<std::string>, Utils::ReturnCodes> SearchMailbox(const std::string &searchKey);
    /**
     * @brief Search local mail directory for mail with full messages (headers + message body). If mail with headers
     * only is present, it will be deleted, so full messages can be received. If full messages are found, their
     * UIDs will be stored, so they can be omitted during fetching of the remote mail
     *
     * @return std::vector<std::string> Contains found UIDs of full messages, so they can be omitted during fetching
     */
    std::vector<std::string> SearchLocalMailDirectoryForFullMail();
    /**
     * @brief Search local mail directory for all mail (headers only + full messages). If any messages are found,
     * their UIDs will be stored, so they can be omitted during fetching of the remote mail
     *
     * @return std::vector<std::string> Contains found UIDs of messages, so they can be omitted during fetching
     */
    std::vector<std::string> SearchLocalMailDirectoryForAll();

  public:
    Session(const std::string &serverHostname, const std::string &port, const std::string &username,
            const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox);
    ~Session();
    /**
     * @brief Get address info about host
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
     * @brief Send message to a server
     *
     * @param message Message to be sent
     * @return Utils::ReturnCodes IMAPCL_SUCCESS if nothing failed, SOCKET_WRITING if writing to the socket failed
     */
    Utils::ReturnCodes SendMessage(const std::string &message);
    /**
     * @brief Receive untagged response from a server
     */
    void ReceiveUntaggedResponse();
    /**
     * @brief Receive tagged response from a server
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
     * @brief Fetch full mail from the remote mailbox.
     *
     * @param newMailOnly Fetch only new mail
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes FetchMail(const bool newMailOnly);
    /**
     * @brief Fetch only headers from the mailbox
     *
     * @param newMailOnly Fetch only new mail
     * @return Utils::ReturnCodes
     */
    Utils::ReturnCodes FetchHeaders(const bool newMailOnly);
    /**
     * @brief Logout user from session
     *
     * @return IMAPCL_SUCCESS if nothing failed
     */
    Utils::ReturnCodes Logout();
};
