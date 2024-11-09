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
     *
     */
    Utils::ReturnCodes SendMessage(const std::string &message);
    /**
     *
     */
    Utils::ReturnCodes EncryptSocket();
    /**
     *
     */
    Utils::ReturnCodes LoadCertificates();
    /**
     *
     */
    Utils::ReturnCodes SelectMailbox(Utils::TypeOfFetch typeOfFetch);
    /**
     *
     */
    std::vector<std::string> SearchMailbox(const std::string &searchKey);

  public:
    EncryptedSession(const std::string &serverHostname, const std::string &port, const std::string &username,
                     const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox,
                     const std::string &certificateFile, const std::string &certificateFileDirectoryPath);
    ~EncryptedSession();
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
    /**
     * @brief Fetch only headers from a mailbox.
     *
     * @return IMAPCL_SUCCESS if nothing failed, CANT_ACCESS_MAILBOX if the mailbox can not be accessed,
     * VALIDITY_FILE_OPEN if the UIDValidity file can not be opened
     */
    Utils::ReturnCodes FetchHeaders(const bool newMailOnly);
    /**
     * @brief Logout user from session
     *
     * @return IMAPCL_SUCCESS if nothing failed
     */
    Utils::ReturnCodes Logout();
};
