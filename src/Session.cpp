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

Session::Session(const std::string &serverHostname, const std::string &port, const std::string &username,
                 const std::string &password, const std::string &outDirectoryPath, const std::string &mailBox)
    : SocketDescriptor(-1), Server(nullptr), ServerHostname(serverHostname), Port(port), Username(username),
      Password(password), Buffer(std::string("", 1024)), FullResponse(""), OutDirectoryPath(outDirectoryPath),
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
    return Utils::IMAPCL_SUCCESS;
}

void Session::ReceiveUntaggedResponse()
{
    while (true)
    {
        unsigned long int received = recv(this->SocketDescriptor, this->Buffer.data(), BUFFER_SIZE, 0);
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

void Session::ReceiveTaggedResponse()
{
    while (true)
    {
        unsigned long int received = recv(this->SocketDescriptor, this->Buffer.data(), BUFFER_SIZE, 0);
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
    this->ReceiveUntaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "\\*\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Authenticate()
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

Utils::ReturnCodes Session::ValidateMailbox()
{
    std::regex validityRegex("UIDVALIDITY\\s([0-9]+)");
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
                // Clearing out local mail directory, because UIDValidity file needs to be update
                // and mail will need to be redownloaded
                for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
                {
                    std::string fileName = entry.path();
                    std::regex messageUIDRegex =
                        std::regex("([0-9]+)_" + this->MailBox + "_" + this->ServerHostname + "_(.+)");
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
    // Selecting mailbox
    this->SendMessage("SELECT " + this->MailBox);
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Can't access mailbox");

    // Checking UIDValidity of the mailbox
    if ((this->ReturnCode = this->ValidateMailbox()))
        return this->ReturnCode;

    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

std::tuple<std::vector<std::string>, Utils::ReturnCodes> Session::SearchMailbox(const std::string &searchKey)
{
    // Searching for all mail in selected mailbox
    if ((this->ReturnCode = this->SendMessage("UID SEARCH " + searchKey)))
        return {{}, this->ReturnCode};
    this->ReceiveTaggedResponse();
    Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK");
    std::regex regex("\\b[0-9]+");
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

std::vector<std::string> Session::SearchLocalMailDirectoryForFullMail()
{
    std::vector<std::string> localMessagesUIDs;
    for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
    {
        std::string fileName = entry.path();
        std::regex messageUIDRegex("([0-9]+)(.+)");
        std::regex messageHeadersRegex("([0-9]+)(.+)(_h)");
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
        std::regex messageUIDRegex("([0-9]+)(.+)");
        std::smatch messageUIDMatch;
        if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
            localMessagesUIDs.push_back(messageUIDMatch[1]);
    }
    return localMessagesUIDs;
}

Utils::ReturnCodes Session::FetchMail(const bool newMailOnly)
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
    // Retrieving sizes of mail
    // TODO: Pass this num to the ReceiveTaggedResponse() to check if send size matches
    unsigned int numOfDownloaded = 0;
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
    std::cout << "Downloaded: " << numOfDownloaded << " message(s) from " << this->MailBox << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::FetchHeaders(const bool newMailOnly)
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
    std::cout << "Downloaded: " << numOfDownloaded << " header(s) from " << this->MailBox << "\n";
    return Utils::IMAPCL_SUCCESS;
}
Utils::ReturnCodes Session::Logout()
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
