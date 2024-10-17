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

#include "../include/Message.h"
#include "../include/Session.h"

Session::Session(const std::string &username, const std::string &password, const std::string &outDirectoryPath,
                 const std::string &mailBox)
    : SocketDescriptor(-1), Server(nullptr), Username(username), Password(password), Buffer(std::string("", 1024)),
      FullResponse(""), OutDirectoryPath(outDirectoryPath), MailBox(mailBox), CurrentTagNumber(1),
      ReturnCode(Utils::IMAPCL_SUCCESS)
{
}

Session::~Session()
{
#ifdef DEBUG
    Utils::PrintDebug("Cleaning up...");
#endif
    if (this->Server != nullptr)
        freeaddrinfo(this->Server);
    if (this->SocketDescriptor > 0)
    {
        shutdown(this->SocketDescriptor, SHUT_RDWR);
        close(this->SocketDescriptor);
    }
#ifdef DEBUG
    Utils::PrintDebug("Cleaning up done!");
#endif
}

Utils::ReturnCodes Session::GetHostAddressInfo(const std::string &serverAddress, const std::string &port)
{
#ifdef DEBUG
    Utils::PrintDebug("Getting host information...");
#endif
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(serverAddress.c_str(), port.c_str(), &hints, &(this->Server)) != 0)
        return Utils::PrintError(Utils::SERVER_BAD_HOST, "Bad host");
#ifdef DEBUG
    Utils::PrintDebug("Getting host information done!");
#endif
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::CreateSocket()
{
#ifdef DEBUG
    Utils::PrintDebug("Creating socket...");
#endif
    if ((this->SocketDescriptor =
             socket(this->Server->ai_family, this->Server->ai_socktype, this->Server->ai_protocol)) <= 0)
        return Utils::PrintError(Utils::SOCKET_CREATING, "Error creating socket");
#ifdef DEBUG
    Utils::PrintDebug("Creating socket done!");
#endif
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
#ifdef DEBUG
    Utils::PrintDebug("Received untagged message: " + this->FullResponse);
#endif
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
            if (!Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber)))
                break;
        }
    }
#ifdef DEBUG
    Utils::PrintDebug("Received tagged message: " + this->FullResponse);
#endif
    this->Buffer = std::string("", BUFFER_SIZE);
}

void Session::SendMessage(const std::string &message)
{
    std::string messageBuffer = "A" + std::to_string(this->CurrentTagNumber) + " ";
    messageBuffer += message + "\n";
#ifdef DEBUG
    Utils::PrintDebug("Sending message: " + messageBuffer);
#endif
    if (send(this->SocketDescriptor, messageBuffer.c_str(), messageBuffer.length(), 0) == -1)
    {
        // TODO: Error handling
    }
}

Utils::ReturnCodes Session::Connect()
{
#ifdef DEBUG
    Utils::PrintDebug("Connecting to socket...");
#endif
    if (connect(this->SocketDescriptor, Server->ai_addr, Server->ai_addrlen) == -1)
        return Utils::PrintError(Utils::SOCKET_CONNECTING, "Connecting to socket failed");
    this->ReceiveUntaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "\\*\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
#ifdef DEBUG
    Utils::PrintDebug("Connected!");
#endif
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Authenticate()
{
#ifdef DEBUG
    Utils::PrintDebug("Logging in...");
#endif
    this->SendMessage("LOGIN " + this->Username + " " + this->Password);
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
#ifdef DEBUG
            Utils::PrintDebug("Logged in!");
#endif
            this->FullResponse = "";
            this->CurrentTagNumber++;
            return Utils::IMAPCL_SUCCESS;
        }
        return Utils::PrintError(Utils::AUTH_INVALID_CREDENTIALS, "Bad credentials");
    }
    return Utils::IMAPCL_FAILURE;
}

Utils::ReturnCodes Session::ValidateMailbox()
{
    std::regex validityRegex("UIDVALIDITY\\s([0-9]+)");
    std::smatch validityMatch;
    std::regex_search(this->FullResponse, validityMatch, validityRegex);
    std::string UIDValidity = validityMatch[1];
    std::string validityFile = this->OutDirectoryPath + "/." + this->MailBox + "_validity";
#ifdef DEBUG
    Utils::PrintDebug("UIDValidity: " + UIDValidity);
    Utils::PrintDebug("UIDValidity file: " + validityFile);
#endif
    struct stat buffer;
    if (stat(validityFile.c_str(), &buffer) != 0)
    {
#ifdef DEBUG
        Utils::PrintDebug("UIDValidity file not found, creating it...");
#endif
        std::ofstream file(validityFile);
        file << UIDValidity << std::endl;
        file.close();
#ifdef DEBUG
        Utils::PrintDebug("UIDValidity file created!");
#endif
    }
    else
    {
#ifdef DEBUG
        Utils::PrintDebug("UIDValidity file found!");
#endif
        std::ifstream file(validityFile);
        std::string line;
        if (!file.is_open())
            return Utils::PrintError(Utils::VALIDITY_FILE_OPEN, "Could not open validity file");
#ifdef DEBUG
        Utils::PrintDebug("Comparing UIDValidity...");
#endif
        while (std::getline(file, line))
        {
            if (line.compare(validityMatch[1]))
            {
                // Clearing out local mail directory, because UIDValidity file needs to be update
                // and mail will need to be redownloaded
#ifdef DEBUG
                Utils::PrintDebug("Clearing old mail...");
#endif
                for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
                {
                    std::string fileName = entry.path();
                    std::regex messageUIDRegex("([0-9]+)(.+)");
                    std::smatch messageUIDMatch;
                    if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
                        std::filesystem::remove_all(entry.path());
                }
#ifdef DEBUG
                Utils::PrintDebug("Clearing old mail done!");
#endif
#ifdef DEBUG
                Utils::PrintDebug("Updating UIDValidity...");
#endif
                // Updating UIDValidity file to a new value
                std::ofstream file(validityFile);
                file << validityMatch[1] << std::endl;
                file.close();
#ifdef DEBUG
                Utils::PrintDebug("Updating UIDValidity done!");
#endif
                break;
            }
        }
#ifdef DEBUG
        Utils::PrintDebug("Comparing UIDValidity done!");
#endif // DEBUG
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::SelectMailbox()
{
    // Selecting mailbox
    this->SendMessage("SELECT " + this->MailBox);
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Can not access mailbox");

    // Checking UIDValidity of the mailbox
    if ((this->ReturnCode = this->ValidateMailbox()))
        return this->ReturnCode;

    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

std::vector<std::string> Session::SearchMailbox(const std::string &searchKey)
{
    // Searching for all mail in selected mailbox
    this->SendMessage("UID SEARCH " + searchKey);
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

    return messageUIDs;
}

std::vector<std::string> Session::SearchLocalMailDirectory()
{
    std::vector<std::string> localMessagesUIDs;
    for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
    {
        // std::cerr << "MAIL FILE: " << entry.path() << std::endl;
        std::string fileName = entry.path();
        std::regex messageUIDRegex("([0-9]+)(.+)");
        std::smatch messageUIDMatch;
        if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
            localMessagesUIDs.push_back(messageUIDMatch[1]);
    }
    return localMessagesUIDs;
}

Utils::ReturnCodes Session::FetchAllMail()
{
#ifdef DEBUG
    Utils::PrintDebug("Selecting mailbox " + this->MailBox + "...");
#endif
    if ((this->ReturnCode = this->SelectMailbox()))
        return this->ReturnCode;
#ifdef DEBUG
    Utils::PrintDebug("Selected mailbox " + this->MailBox + "!");
#endif
#ifdef DEBUG
    Utils::PrintDebug("Searching mailbox for ALL mail...");
#endif
    std::vector<std::string> messageUIDs = this->SearchMailbox("ALL");
#ifdef DEBUG
    Utils::PrintDebug("Searching mailbox for ALL mail done!");
#endif
#ifdef DEBUG
    Utils::PrintDebug("Searching local mail...");
#endif
    std::vector<std::string> localMessagesUIDs = this->SearchLocalMailDirectory();
#ifdef DEBUG
    Utils::PrintDebug("Searching local mail done!");
#endif
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
        // #ifdef DEBUG
        //         Utils::PrintDebug("Received size: " + this->FullResponse);
        // #endif // DEBUG
        this->FullResponse = "";
        this->CurrentTagNumber++;
// Fetching mail
#ifdef DEBUG
        Utils::PrintDebug("Fetching mail with UID " + x + "...");
#endif
        this->SendMessage("UID FETCH " + x + " BODY[]");
        this->ReceiveTaggedResponse();
        Message message(x, this->FullResponse);
        message.ParseFileName();
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
#ifdef DEBUG
        Utils::PrintDebug("Fetching mail with UID " + x + " done!");
#endif
    }
    std::cout << "Downloaded: " << numOfDownloaded << " file(s)"
              << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Logout()
{
#ifdef DEBUG
    Utils::PrintDebug("Logging out...");
#endif
    this->SendMessage("LOGOUT");
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    this->CurrentTagNumber++;
#ifdef DEBUG
    Utils::PrintDebug("Logged out succesfully!");
#endif
    return Utils::IMAPCL_SUCCESS;
}
