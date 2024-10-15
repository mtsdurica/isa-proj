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
      FullResponse(""), OutDirectoryPath(outDirectoryPath), MailBox(mailBox), CurrentTagNumber(1)
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

Utils::ReturnCodes Session::GetHostAddressInfo(const std::string &serverAddress, const std::string &port)
{
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(serverAddress.c_str(), port.c_str(), &hints, &(this->Server)) != 0)
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
            if (!Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber)))
                break;
        }
    }
    this->Buffer = std::string("", BUFFER_SIZE);
}

void Session::SendMessage(const std::string &message)
{
    std::string messageBuffer = "A" + std::to_string(this->CurrentTagNumber) + " ";
    messageBuffer += message + "\n";
    if (send(this->SocketDescriptor, messageBuffer.c_str(), messageBuffer.length(), 0) == -1)
    {
        // TODO: Error handling
    }
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
            this->FullResponse = "";
            this->CurrentTagNumber++;
            return Utils::IMAPCL_SUCCESS;
        }
        return Utils::PrintError(Utils::AUTH_INVALID_CREDENTIALS, "Bad credentials");
    }
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::ValidateMailbox()
{
    std::regex validityRegex("UIDVALIDITY\\s([0-9]+)");
    std::smatch validityMatch;
    std::regex_search(this->FullResponse, validityMatch, validityRegex);
    std::string validityFile = this->OutDirectoryPath + "/." + this->MailBox + "_validity";
    struct stat buffer;
    if (stat(validityFile.c_str(), &buffer) != 0)
    {
        std::ofstream file(validityFile);
        file << validityMatch[1] << std::endl;
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
            if (!line.compare(validityMatch[1]))
            {
                this->MailBoxValidated = true;
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
        return Utils::PrintError(Utils::CANT_ACCESS_MAILBOX, "Cant access mailbox");

    // Checking UIDValidity of the mailbox
    Utils::ReturnCodes returnCode;
    if ((returnCode = this->ValidateMailbox()))
        return returnCode;

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

Utils::ReturnCodes Session::FetchAllMail()
{
    Utils::ReturnCodes returnCode;
    if ((returnCode = this->SelectMailbox()))
        return returnCode;
    std::vector<std::string> messageUIDs = this->SearchMailbox("ALL");
    std::vector<std::string> localMessagesUIDs;
    for (const auto &entry : std::filesystem::directory_iterator(this->OutDirectoryPath))
    {
        // std::cerr << "MAIL FILE: " << entry.path() << std::endl;
        std::string fileName = entry.path();
        std::regex messageUIDRegex("([0-9])(.+)");
        std::smatch messageUIDMatch;
        if (std::regex_search(fileName, messageUIDMatch, messageUIDRegex))
            localMessagesUIDs.push_back(messageUIDMatch[1]);
    }
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
        this->SendMessage("UID FETCH " + x + "RFC822.SIZE");
        this->ReceiveTaggedResponse();
        Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK");
        this->FullResponse = "";
        this->CurrentTagNumber++;
        // Fetching mail
        this->SendMessage("UID FETCH " + x + " BODY[]");
        this->ReceiveTaggedResponse();
        Message message(x, this->FullResponse);
        message.ParseFileName();
        message.ParseMessageBody();
        message.DumpToFile(this->OutDirectoryPath);
        this->FullResponse = "";
        this->CurrentTagNumber++;
        numOfDownloaded++;
    }
    std::cout << "Downloaded: " << numOfDownloaded << " file(s)" << "\n";
    return Utils::IMAPCL_SUCCESS;
}

Utils::ReturnCodes Session::Logout()
{
    this->SendMessage("LOGOUT");
    this->ReceiveTaggedResponse();
    if (Utils::ValidateResponse(this->FullResponse, "A" + std::to_string(this->CurrentTagNumber) + "\\sOK"))
        return Utils::PrintError(Utils::INVALID_RESPONSE, "Response is invalid");
    this->FullResponse = "";
    this->CurrentTagNumber++;
    return Utils::IMAPCL_SUCCESS;
}

int Session::GetSocketDescriptor()
{
    return this->SocketDescriptor;
}
