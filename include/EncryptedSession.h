#pragma once

#include "Session.h"
#include "Utils.h"
#include <openssl/ssl.h>
#include <string>
#include <vector>

class EncryptedSession final : public Session
{
  protected:
    SSL_CTX *SecureContext;
    SSL *SecureConnection;
    std::string CertificateFile;
    std::string CertificateFileDirectoryPath;
    /**
     * @brief Send message to a server
     *
     * @param message Message to be sent
     * @return Utils::ReturnCodes IMAPCL_SUCCESS if nothing failed, SOCKET_WRITING if writing to the socket failed
     */
    Utils::ReturnCodes SendMessage(const std::string &message);
    /**
     * @brief Encrypt socket for encrypted communication
     *
     * @return Utils::ReturnCodes IMAPCL_SUCCESS if nothing failed, SSL_CONTEXT_CREATE if creating SSL context failed,
     * CERTIFICATE_ERROR if loading certificates failed, SSL_CONNECTION_ERROR if connection creation failed,
     * SSL_SET_DESCRIPTOR if setting socket descriptor to the SSL context failed, SSL_HANDSHAKE_FAILED if the SSL
     * handshake failed
     */
    Utils::ReturnCodes EncryptSocket();
    /**
     * @brief Load certificate directory
     *
     * @return Utils::ReturnCodes IMAPCL_SUCCESS if nothing failed, CERTIFICATE_ERROR if loading certificate directory
     * failed
     */
    Utils::ReturnCodes LoadCertificates();
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

  public:
    EncryptedSession(const std::string &serverHostname, const std::string &port, const std::string &username,
                     const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox,
                     const std::string &certificateFile, const std::string &certificateFileDirectoryPath);
    ~EncryptedSession();
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
     * @brief Fetch all mail from a mailbox.
     *
     * @param newMailOnly Fetch only new mail
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes FetchMail(const bool newMailOnly);
    /**
     * @brief Fetch only headers from a mailbox.
     *
     * @param newMailOnly Fetch only new mail
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes FetchHeaders(const bool newMailOnly);
    /**
     * @brief Logout user from session
     *
     * @return IMAPCL_SUCCESS if nothing failed, SOCKET_WRITING if writing to a socket failed, INVALID_RESPONSE if
     * received response from server was invalid
     */
    Utils::ReturnCodes Logout();
};
