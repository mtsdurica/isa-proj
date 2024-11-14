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

#include <openssl/evp.h>
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

void EncryptedSession::ReceiveUntaggedResponse()
{
    while (true)
    {
        unsigned long int received = SSL_read(this->SecureConnection, this->Buffer.data(), BUFFER_SIZE);
        if (received != 0)
        {
            this->FullResponse += this->Buffer.substr(0, received);
            // TODO: Rework this
            if (!Utils::ValidateResponse(this->FullResponse, "\\*"))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
}

void EncryptedSession::ReceiveTaggedResponse()
{
    while (true)
    {
        unsigned long int received = SSL_read(this->SecureConnection, this->Buffer.data(), BUFFER_SIZE);
        if (received != 0)
        {
            this->FullResponse += this->Buffer.substr(0, received);
            // Keep listening on the port until 'current tag' is present
            // Weird bug happened, when fetching from imap.stud.fit.vutbr.cz, sometimes this function ignored parts of
            // the response string passed to it; it was not happening consistently, probably has to do something with
            // regex, but this workaround worked in my testing
            if (!Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK") ||
                !Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sNO") ||
                !Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sBAD"))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
}

Utils::ReturnCodes EncryptedSession::LoadCertificates()
{
    if (!SSL_CTX_load_verify_dir(this->SecureContext, this->CertificateFileDirectoryPath.c_str()))
        return Utils::CERTIFICATE_ERROR;
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::EncryptSocket()
{
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
    this->FullResponse = "";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Authenticate()
{
    if ((this->ReturnCode = this->SendMessage("LOGIN " + this->Username + " " + this->Password)))
        return this->ReturnCode;
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sNO"))
    {
        if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        {
            Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
            return Utils::INVALID_RESPONSE;
        }
        else
        {
            this->FullResponse = "";
            this->CurrentTagNumber++;
            return Utils::IMAPCL_SUCCESS;
        }
    }
    else
        return Utils::PrintError(Utils::AUTH_INVALID_CREDENTIALS, "Bad credentials");

    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::SelectMailbox()
{
    // Selecting mailbox
    if ((this->ReturnCode = this->SendMessage("SELECT " + this->MailBox)))
        return this->ReturnCode;
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Can't access mailbox");

    // Checking UIDValidity of the mailbox
    if ((this->ReturnCode = this->ValidateMailbox()))
        return ReturnCode;

    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

std::tuple<std::vector<std::string>, Utils::ReturnCodes> EncryptedSession::SearchMailbox(const std::string &searchKey)
{
    // Searching for all mail in selected mailbox
    if ((this->ReturnCode = this->SendMessage("UID SEARCH " + searchKey)))
        return {{}, this->ReturnCode};
    this->ReceiveTaggedResponse();
    Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK");
    // Extracting line with UIDs of mail
    std::regex removeSecondLine("A" + std::to_string(this->CurrentTagNumber) + "[\\s\\S]+");
    this->FullResponse = std::regex_replace(this->FullResponse, removeSecondLine, "");
    std::regex regex("[0-9]+");
    std::smatch match;
    std::string::const_iterator start(this->FullResponse.cbegin());
    std::vector<std::string> messageUIDs;
    while (std::regex_search(start, this->FullResponse.cend(), match, regex))
    {
        messageUIDs.push_back(match[0]);
        start = match.suffix().first;
    }
    // TODO: Handle empty mailbox
    this->FullResponse = "";
    this->CurrentTagNumber++;

    return {messageUIDs, Utils::IMAPCL_SUCCESS};
}

Utils::ReturnCodes EncryptedSession::FetchMail(const bool newMailOnly)
{
    if ((this->ReturnCode = this->SelectMailbox()))
        return this->ReturnCode;
    std::vector<std::string> messageUIDs;
    if (newMailOnly)
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("UNSEEN");
    else
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("ALL");
    if (this->ReturnCode)
        return this->ReturnCode;
    std::vector<std::string> localMessagesUIDs = this->SearchLocalMailDirectoryForFullMail();
    int numOfDownloaded = 0;
    for (auto x : messageUIDs)
    {
        bool mailNotToBeDownloaded = false;
        for (auto m : localMessagesUIDs)
            if (!x.compare(m))
                mailNotToBeDownloaded = true;
        if (mailNotToBeDownloaded)
            continue;
        // Retrieving size of each message
        if ((this->ReturnCode = this->SendMessage("UID FETCH " + x + " RFC822.SIZE")))
            return this->ReturnCode;
        this->ReceiveTaggedResponse();
        Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK");
        std::regex rfcSizeRegex("RFC822.SIZE\\s([0-9]+)");
        std::smatch rfcSizeMatch;
        std::regex_search(this->FullResponse, rfcSizeMatch, rfcSizeRegex);
        std::string rfcSize = rfcSizeMatch[1];
        this->FullResponse = "";
        this->CurrentTagNumber++;
        // Fetching mail
        if ((this->ReturnCode = this->SendMessage("UID FETCH " + x + " BODY[]")))
            return this->ReturnCode;
        this->ReceiveTaggedResponse();
        Message message(x, this->FullResponse, stoi(rfcSize));
        message.ParseFileName(this->ServerHostname, this->MailBox);
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    std::cout << "Downloaded: " << numOfDownloaded << " message(s) from " << this->MailBox << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::FetchHeaders(const bool newMailOnly)
{
    if ((this->ReturnCode = this->SelectMailbox()))
        return this->ReturnCode;
    std::vector<std::string> messageUIDs;
    if (newMailOnly)
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("UNSEEN");
    else
        std::tie(messageUIDs, this->ReturnCode) = this->SearchMailbox("ALL");
    if (this->ReturnCode)
        return this->ReturnCode;
    std::vector<std::string> localMessagesUIDs = this->SearchLocalMailDirectoryForAll();
    // Retrieving sizes of mail
    // TODO: Pass this num to the ReceiveTaggedResponse() to check if send size matches
    int numOfDownloaded = 0;
    for (auto x : messageUIDs)
    {
        bool mailNotToBeDownloaded = false;
        for (auto m : localMessagesUIDs)
            if (!x.compare(m))
                mailNotToBeDownloaded = true;
        if (mailNotToBeDownloaded)
            continue;
        // Fetching mail
        if ((this->ReturnCode = this->SendMessage("UID FETCH " + x + " BODY.PEEK[HEADER]")))
            return this->ReturnCode;
        this->ReceiveTaggedResponse();
        HeaderMessage message(x, this->FullResponse);
        message.ParseFileName(this->ServerHostname, this->MailBox);
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    std::cout << "Downloaded: " << numOfDownloaded << " header(s) from " << this->MailBox << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Logout()
{
    if ((this->ReturnCode = this->SendMessage("LOGOUT")))
        return this->ReturnCode;
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}
