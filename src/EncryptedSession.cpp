/**
 * @file EncryptedSession.cpp
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains definitions of EncryptedSession class methods
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "../include/EncryptedSession.h"

#include <openssl/ssl.h>
#include <string>

#include "../include/HeaderMessage.h"
#include "../include/Message.h"

EncryptedSession::EncryptedSession(const std::string &serverHostname, const std::string &port,
                                   const std::string &username, const std::string &password,
                                   const std::string &outDirectoryPath, const std::string &mailBox,
                                   const std::string &certificateFile, const std::string &certificateFileDirectoryPath)
    : Session(serverHostname, port, username, password, outDirectoryPath, mailBox), CertificateFile(certificateFile),
      CertificateFileDirectoryPath(certificateFileDirectoryPath)
{
}

EncryptedSession::~EncryptedSession()
{
    SSL_shutdown(this->SecureConnection);
    SSL_free(this->SecureConnection);
    SSL_CTX_free(this->SecureContext);
}

Utils::ReturnCodes EncryptedSession::ReceiveUntaggedResponse()
{
    while (true)
    {
        unsigned long int received = SSL_read(this->SecureConnection, this->Buffer.data(), BUFFER_SIZE);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return Utils::PrintError(Utils::SOCKET_TIMED_OUT, "Timed out");
        if (received != 0)
        {
            this->FullResponse += this->Buffer.substr(0, received);
            // Keep listening on the port until '*' + OK/NO/BAD is present, so we can stop reading
            if (!Utils::ValidateResponse(this->FullResponse, "\\*\\sOK") ||
                !Utils::ValidateResponse(this->FullResponse, "\\*\\sNO") ||
                !Utils::ValidateResponse(this->FullResponse, "\\*\\sBAD"))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::ReceiveTaggedResponse()
{
    while (true)
    {
        unsigned long int received = SSL_read(this->SecureConnection, this->Buffer.data(), BUFFER_SIZE);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return Utils::PrintError(Utils::SOCKET_TIMED_OUT, "Timed out");
        if (received != 0)
        {
            this->FullResponse += this->Buffer.substr(0, received);
            // Keep listening on the port until 'current tag' + OK/NO/BAD is present, so we can stop reading
            if (!Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK") ||
                !Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sNO") ||
                !Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sBAD"))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::LoadCertificates()
{
    if (!SSL_CTX_load_verify_dir(this->SecureContext, this->CertificateFileDirectoryPath.c_str()))
        return Utils::CERTIFICATE_ERROR;
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::EncryptSocket()
{
#ifdef DEBUG
    std::cerr << "Encrypting socket... ";
#endif
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    // Creating SSL context
    if ((this->SecureContext = SSL_CTX_new(TLS_client_method())) == nullptr)
        return Utils::PrintError(Utils::SSL_CONTEXT_CREATE, "Failed creating SSL context");
    // Verifying certificate file
    if (this->CertificateFile != "")
        if (!SSL_CTX_load_verify_file(this->SecureContext, this->CertificateFile.c_str()))
            return Utils::PrintError(Utils::CERTIFICATE_ERROR, "Certificate file error");
    // Verifying certificate directory
    if (!SSL_CTX_load_verify_dir(this->SecureContext, this->CertificateFileDirectoryPath.c_str()))
        return Utils::PrintError(Utils::CERTIFICATE_ERROR, "Certificate directory error");
    // Creating SSL connection from context
    if ((this->SecureConnection = SSL_new(this->SecureContext)) == nullptr)
        return Utils::PrintError(Utils::SSL_CONNECTION_CREATE, "Failed creating SSL connection");
    // Setting BSD socket descriptor into SSL
    if (!SSL_set_fd(this->SecureConnection, this->SocketDescriptor))
        return Utils::PrintError(Utils::SSL_SET_DESCRIPTOR, "Failed setting socket descriptor to SSL");
    // Connecting through SSL
    if (SSL_connect(this->SecureConnection) <= 0)
        return Utils::PrintError(Utils::SSL_HANDSHAKE_FAILED, "Failed SSL handshake");
#ifdef DEBUG
    std::cerr << "DONE" << std::endl;
#endif
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::SendMessage(const std::string &message)
{
    std::string messageBuffer = "A" + std::to_string(this->CurrentTagNumber) + " ";
    messageBuffer += message + "\n";
    if (SSL_write(this->SecureConnection, messageBuffer.c_str(), messageBuffer.length()) <= 0)
        return Utils::PrintError(Utils::SOCKET_WRITING, "Failed writing to a socket");
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Connect()
{
    if (connect(this->SocketDescriptor, Server->ai_addr, Server->ai_addrlen) == -1)
        return Utils::PrintError(Utils::SOCKET_CONNECTING, "Connecting to socket failed");
    if ((this->ReturnCode = this->EncryptSocket()))
        return this->ReturnCode;
    if ((this->ReturnCode = this->ReceiveUntaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "\\*\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Authenticate()
{
    if ((this->ReturnCode = this->SendMessage("LOGIN " + this->Username + " " + this->Password)))
        return this->ReturnCode;
    if ((this->ReturnCode = this->ReceiveTaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sNO"))
    {
        if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        {
            this->CurrentTagNumber++;
            this->Logout();
            return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
        }
        else
        {
            this->FullResponse = "";
            this->CurrentTagNumber++;
            return Utils::IMAPCL_SUCCESS;
        }
    }
    else
    {
        this->CurrentTagNumber++;
        this->Logout();
        return Utils::PrintError(Utils::AUTH_INVALID_CREDENTIALS, "Bad credentials");
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::SelectMailbox()
{
#ifdef DEBUG
    std::cerr << "Selecting mailbox " << this->MailBox << "... ";
#endif
    // Selecting mailbox
    if ((this->ReturnCode = this->SendMessage("SELECT " + this->MailBox)))
        return this->ReturnCode;
    if ((this->ReturnCode = this->ReceiveTaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
    {
        this->CurrentTagNumber++;
        this->Logout();
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Can't access mailbox");
    }
#ifdef DEBUG
    std::cerr << "DONE" << std::endl;
#endif
#ifdef DEBUG
    std::cerr << "Checking validity... ";
#endif
    // Checking UIDValidity of the mailbox
    if ((this->ReturnCode = this->ValidateMailbox()))
    {
        this->CurrentTagNumber++;
        this->Logout();
        return this->ReturnCode;
    }
#ifdef DEBUG
    std::cerr << "DONE" << std::endl;
#endif
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

std::tuple<std::vector<std::string>, Utils::ReturnCodes> EncryptedSession::SearchMailbox(const std::string &searchKey)
{
    std::vector<std::string> messageUIDs;
#ifdef DEBUG
    std::cerr << "Searching for " << searchKey << "... ";
#endif
    // Searching for all mail in selected mailbox
    if ((this->ReturnCode = this->SendMessage("UID SEARCH " + searchKey)))
        return {messageUIDs, this->ReturnCode};
    if ((this->ReturnCode = this->ReceiveTaggedResponse()))
        return {messageUIDs, this->ReturnCode};
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
    {
        this->CurrentTagNumber++;
        this->Logout();
        return {messageUIDs, Utils::PrintError(Utils::INVALID_RESPONSE, "Invalid response")};
    }
#ifdef DEBUG
    std::cerr << "DONE" << std::endl;
#endif
    // Extracting line with UIDs of mail
    std::regex removeSecondLine("A" + std::to_string(this->CurrentTagNumber) + "[\\s\\S]+",
                                std::regex_constants::icase);
    this->FullResponse = std::regex_replace(this->FullResponse, removeSecondLine, "");
    std::regex regex("[0-9]+", std::regex_constants::icase);
    std::smatch match;
    std::string::const_iterator start(this->FullResponse.cbegin());
    while (std::regex_search(start, this->FullResponse.cend(), match, regex))
    {
        messageUIDs.push_back(match[0]);
        start = match.suffix().first;
    }

    this->FullResponse = "";
    this->CurrentTagNumber++;
    return {messageUIDs, Utils::IMAPCL_SUCCESS};
}

Utils::ReturnCodes EncryptedSession::FetchMail(const bool headersOnly, const bool newMailOnly)
{
    if ((this->ReturnCode = this->SelectMailbox()))
        return this->ReturnCode;
    std::vector<std::string> messageUIDs;
    if (newMailOnly)
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("NEW");
    else
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("ALL");
    if (this->ReturnCode == Utils::SOCKET_WRITING)
        return this->ReturnCode;
    else if (this->ReturnCode > 0)
    {
        this->CurrentTagNumber++;
        this->Logout();
        return this->ReturnCode;
    }
    std::vector<std::string> localMessagesUIDs;
    if (headersOnly)
        localMessagesUIDs = this->SearchLocalMailDirectoryForAll();
    else
        localMessagesUIDs = this->SearchLocalMailDirectoryForFullMail();
    unsigned int numOfDownloaded = 0;
    for (auto x : messageUIDs)
    {
        bool mailNotToBeDownloaded = false;
        for (auto m : localMessagesUIDs)
            if (!x.compare(m))
                mailNotToBeDownloaded = true;
        if (mailNotToBeDownloaded)
            continue;
        std::unique_ptr<Message> message;
        if (headersOnly)
        {

            // Fetching headers
            this->SendMessage("UID FETCH " + x + " BODY.PEEK[HEADER]");
#ifdef DEBUG
            std::cerr << "Fetching headers of message with UID: " << x << " in progress...";
#endif
            if ((this->ReturnCode = this->ReceiveTaggedResponse()))
                return this->ReturnCode;
            if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
            {
                this->CurrentTagNumber++;
                this->Logout();
                return Utils::PrintError(Utils::INVALID_RESPONSE, "Invalid response");
            }
#ifdef DEBUG
            std::cerr << " DONE" << std::endl;
#endif
            message = std::make_unique<HeaderMessage>(x, this->FullResponse);
        }
        else
        {
            // Fetching full messages
            // Retrieving size of each message
            this->SendMessage("UID FETCH " + x + " RFC822.SIZE");
            if ((this->ReturnCode = this->ReceiveTaggedResponse()))
                return this->ReturnCode;
            if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
            {
                this->CurrentTagNumber++;
                this->Logout();
                return Utils::PrintError(Utils::INVALID_RESPONSE, "Invalid response");
            }
            std::regex rfcSizeRegex("RFC822.SIZE\\s([0-9]+)", std::regex_constants::icase);
            std::smatch rfcSizeMatch;
            std::regex_search(this->FullResponse, rfcSizeMatch, rfcSizeRegex);
            std::string rfcSize = rfcSizeMatch[1];
            this->FullResponse = "";
            this->CurrentTagNumber++;
            // Fetching mail
            this->SendMessage("UID FETCH " + x + " BODY[]");
#ifdef DEBUG
            std::cerr << "Fetching full message with UID: " << x << " in progress...";
#endif
            if ((this->ReturnCode = this->ReceiveTaggedResponse()))
                return this->ReturnCode;
            if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
            {
                this->CurrentTagNumber++;
                this->Logout();
                return Utils::PrintError(Utils::INVALID_RESPONSE, "Invalid response");
            }
#ifdef DEBUG
            std::cerr << " DONE" << std::endl;
#endif
            message = std::make_unique<Message>(x, this->FullResponse, stoi(rfcSize));
        }
        message->ParseFileName(this->ServerHostname, this->MailBox);
        message->ParseMessageBody();
        message->DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    if (headersOnly)
        if (newMailOnly)
            std::cout << "Downloaded: " << numOfDownloaded << " new header(s) from " << this->MailBox << "\n";
        else
            std::cout << "Downloaded: " << numOfDownloaded << " header(s) from " << this->MailBox << "\n";
    else
    {
        if (newMailOnly)
            std::cout << "Downloaded: " << numOfDownloaded << " new message(s) from " << this->MailBox << "\n";
        else
            std::cout << "Downloaded: " << numOfDownloaded << " message(s) from " << this->MailBox << "\n";
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Logout()
{
    if ((this->ReturnCode = this->SendMessage("LOGOUT")))
        return this->ReturnCode;
    if ((this->ReturnCode = this->ReceiveTaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}
