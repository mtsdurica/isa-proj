/**
 * @file Session.cpp
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains definitions of Session class methods
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <sys/socket.h>

#include "../include/HeaderMessage.h"
#include "../include/Message.h"
#include "../include/Session.h"

Session::Session()
{
}

Session::Session(const std::string &serverHostname, const std::string &port, const std::string &username,
                 const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox)
    : SocketDescriptor(-1), Server(nullptr), ServerHostname(serverHostname), Port(port), Username(username),
      Password(password), Buffer(std::string("", BUFFER_SIZE)), FullResponse(""), OutDirectoryPath(outDirectoryPath),
      MailBox(mailBox), CurrentTagNumber(1), ReturnCode(Utils::IMAPCL_SUCCESS)
{
}

Session::~Session()
{
    if (this->Server != nullptr)
        freeaddrinfo(this->Server);
    if (this->SocketDescriptor > 0)
    {
        shutdown(this->SocketDescriptor, SHUT_RDWR);
        close(this->SocketDescriptor);
    }
}

Utils::ReturnCodes Session::GetHostAddressInfo()
{
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(this->ServerHostname.c_str(), this->Port.c_str(), &hints, &(this->Server)) != 0)
        return Utils::PrintError(Utils::SERVER_BAD_HOST, "Bad host");
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::CreateSocket()
{
    if ((this->SocketDescriptor =
             socket(this->Server->ai_family, this->Server->ai_socktype, this->Server->ai_protocol)) <= 0)
        return Utils::PrintError(Utils::SOCKET_CREATING, "Error creating socket");
    // https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
    struct timeval timeout = {10, 0};
    setsockopt(this->SocketDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::ReceiveUntaggedResponse()
{
    while (true)
    {
        unsigned long int received = recv(this->SocketDescriptor, this->Buffer.data(), BUFFER_SIZE, 0);
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

Utils::ReturnCodes Session::ReceiveTaggedResponse()
{
    while (true)
    {
        unsigned long int received = recv(this->SocketDescriptor, this->Buffer.data(), BUFFER_SIZE, 0);
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

Utils::ReturnCodes Session::SendMessage(const std::string &message)
{
    std::string messageBuffer = "A" + std::to_string(this->CurrentTagNumber) + " ";
    messageBuffer += message + "\n";
    if (send(this->SocketDescriptor, messageBuffer.c_str(), messageBuffer.length(), 0) == -1)
        return Utils::PrintError(Utils::SOCKET_WRITING, "Failed writing to a socket");
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Connect()
{
    if (connect(this->SocketDescriptor, Server->ai_addr, Server->ai_addrlen) == -1)
        return Utils::PrintError(Utils::SOCKET_CONNECTING, "Connecting to socket failed");
    if ((this->ReturnCode = this->ReceiveUntaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "\\*\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Authenticate()
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

Utils::ReturnCodes Session::ValidateMailbox()
{
    std::regex validityRegex("UIDVALIDITY\\s([0-9]+)", std::regex_constants::icase);
    std::smatch validityMatch;
    std::regex_search(this->FullResponse, validityMatch, validityRegex);
    std::string UIDValidity = validityMatch[1];
    std::string validityFile = this->OutDirectoryPath + "/." + this->ServerHostname + "_" + this->MailBox + "_validity";
    struct stat buffer;
    if (stat(validityFile.c_str(), &buffer) != 0)
    {
        std::ofstream file(validityFile);
        file << UIDValidity << std::endl;
        file.close();
    }
    else
    {
        std::ifstream file(validityFile);
        std::string line;
        if (!file.is_open())
            return Utils::PrintError(Utils::VALIDITY_FILE_OPEN, "Could not open validity file");
        while (std::getline(file, line))
        {
            if (line.compare(validityMatch[1]))
            {
                // Clearing out local mail directory, because UIDValidity file needs to be updated
                // and mail will need to be redownloaded
                for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
                {
                    std::string fileName = entry.path();
                    std::regex messageUIDRegex =
                        std::regex("([0-9]+)_" + this->MailBox + "_" + this->ServerHostname + "_(.+)",
                                   std::regex_constants::icase);
                    std::smatch messageUIDMatch;
                    if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
                        std::filesystem::remove_all(entry.path());
                }
                // Updating UIDValidity file to a new value
                std::ofstream file(validityFile);
                file << validityMatch[1] << std::endl;
                file.close();
                break;
            }
        }
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::SelectMailbox()
{
#ifdef DEBUG
    std::cerr << "Selecting mailbox " << this->MailBox << "... ";
#endif
    // Selecting mailbox
    this->SendMessage("SELECT " + this->MailBox);
    if ((this->ReturnCode = this->ReceiveTaggedResponse()))
        return this->ReturnCode;
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
    {
        this->CurrentTagNumber++;
        this->Logout();
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Invalid response");
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

std::tuple<std::vector<std::string>, Utils::ReturnCodes> Session::SearchMailbox(const std::string &searchKey)
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
    std::regex removeSecondLine("A" + std::to_string(this->CurrentTagNumber) + "[\\s\\S]+");
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

std::vector<std::string> Session::SearchLocalMailDirectoryForFullMail()
{
    std::vector<std::string> localMessagesUIDs;
    for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
    {
        std::string fileName = entry.path();
        std::regex messageUIDRegex("([0-9]+)(.+)", std::regex_constants::icase);
        std::regex messageHeadersRegex("([0-9]+)(.+)(_h)", std::regex_constants::icase);
        std::smatch messageUIDMatch;
        std::smatch messageHeadersMatch;
        if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
        {
            if (std::regex_search(fileName, messageHeadersMatch, messageHeadersRegex))
            {
                std::filesystem::remove_all(entry.path());
            }
            else
                localMessagesUIDs.push_back(messageUIDMatch[1]);
        }
    }
    return localMessagesUIDs;
}

std::vector<std::string> Session::SearchLocalMailDirectoryForAll()
{
    std::vector<std::string> localMessagesUIDs;
    for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
    {
        std::string fileName = entry.path();
        std::regex messageUIDRegex("([0-9]+)(.+)", std::regex_constants::icase);
        std::smatch messageUIDMatch;
        if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
            localMessagesUIDs.push_back(messageUIDMatch[1]);
    }
    return localMessagesUIDs;
}

Utils::ReturnCodes Session::FetchMail(const bool headersOnly, const bool newMailOnly)
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

Utils::ReturnCodes Session::Logout()
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
