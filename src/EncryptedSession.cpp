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
            if (!Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber)))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
}

Utils::ReturnCodes EncryptedSession::EncryptSocket()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    // Creating SSL context
    if ((this->SecureContext = SSL_CTX_new(TLS_client_method())) == nullptr)
        return Utils::PrintError(Utils::SSL_CONTEXT_CREATE, "Failed creating SSL context");
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
        return Utils::PrintError(Utils::AUTH_INVALID_CREDENTIALS, "Bad credentials");
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::SelectMailbox(Utils::TypeOfFetch typeOfFetch)
{
    // Selecting mailbox
    if ((this->ReturnCode = this->SendMessage("SELECT " + this->MailBox)))
        return this->ReturnCode;
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Cant access mailbox");

    // Checking UIDValidity of the mailbox
    if ((this->ReturnCode = this->ValidateMailbox(typeOfFetch)))
        return ReturnCode;

    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

std::vector<std::string> EncryptedSession::SearchMailbox(const std::string &searchKey)
{
    // Searching for all mail in selected mailbox
    this->SendMessage("UID SEARCH " + searchKey);
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

    return messageUIDs;
}

Utils::ReturnCodes EncryptedSession::FetchMail(const bool newMailOnly)
{
    if ((this->ReturnCode = this->SelectMailbox(Utils::FETCH_ALL)))
        return this->ReturnCode;
    std::vector<std::string> messageUIDs;
    if (newMailOnly)
        messageUIDs = this->SearchMailbox("UNSEEN");
    else
        messageUIDs = this->SearchMailbox("ALL");
    std::vector<std::string> localMessagesUIDs = this->SearchLocalMailDirectory();
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
        // Retrieving size of each message
        this->SendMessage("UID FETCH " + x + " RFC822.SIZE");
        this->ReceiveTaggedResponse();
        Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK");
        std::regex rfcSizeRegex("RFC822.SIZE\\s([0-9]+)");
        std::smatch rfcSizeMatch;
        std::regex_search(this->FullResponse, rfcSizeMatch, rfcSizeRegex);
        std::string rfcSize = rfcSizeMatch[1];
        this->FullResponse = "";
        this->CurrentTagNumber++;
        // Fetching mail
        this->SendMessage("UID FETCH " + x + " BODY[]");
        this->ReceiveTaggedResponse();
        Message message(x, this->FullResponse, stoi(rfcSize));
        message.ParseFileName(this->ServerHostname, this->MailBox);
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    std::cout << "Downloaded: " << numOfDownloaded << " file(s)" << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::FetchHeaders(const bool newMailOnly)
{
    if ((this->ReturnCode = this->SelectMailbox(Utils::FETCH_HEADERS)))
        return this->ReturnCode;
    std::vector<std::string> messageUIDs;
    if (newMailOnly)
        messageUIDs = this->SearchMailbox("UNSEEN");
    else
        messageUIDs = this->SearchMailbox("ALL");
    std::vector<std::string> localMessagesUIDs = this->SearchLocalMailDirectory();
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
        this->SendMessage("UID FETCH " + x + " BODY.PEEK[HEADER]");
        this->ReceiveTaggedResponse();
        HeaderMessage message(x, this->FullResponse);
        message.ParseFileName(this->ServerHostname, this->MailBox);
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    std::cout << "Downloaded: " << numOfDownloaded << " file(s)"
              << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes EncryptedSession::Logout()
{
    this->SendMessage("LOGOUT");
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}
