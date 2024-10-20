#include "../include/Message.h"

#include <fstream>
#include <iostream>
#include <regex>

Message::Message(const std::string &messageUID, const std::string &responseString, int rfcSize)
    : MessageUID(messageUID), ResponseString(responseString), FileName(""), MessageBody(""), RfcSize(rfcSize)
{
}

Message::~Message() = default;

void Message::ParseFileName(const std::string &serverHostname, const std::string &mailbox)
{

    this->FileName = this->MessageUID + "_";
    this->FileName += mailbox + "_";
    this->FileName += serverHostname + "_";
    std::regex subjectRegex("(Subject:\\s)(.+)");
    std::smatch match;
    std::regex_search(this->ResponseString, match, subjectRegex);
    this->FileName += match[2];
    this->FileName += "_";
    std::regex fromRegex("(From:\\s.*<)(.+)>");
    std::regex_search(this->ResponseString, match, fromRegex);
    this->FileName += match[2];
    this->FileName += "_";
    // Hashing Message-ID
    std::regex messageIDRegex("(Message-ID:\\s.*<)(.+)>");
    std::regex_search(this->ResponseString, match, messageIDRegex);
    std::hash<std::string> hasher;
    this->FileName += std::to_string(hasher(match[2]));
    std::replace(this->FileName.begin(), this->FileName.end(), ' ', '_');
    this->FileName += ".eml";
}

void Message::ParseMessageBody()
{
    std::cerr << "START\n" << this->ResponseString << "\nEND\n";
    // Removing start and end of the fetch response
    std::regex startAndEndRegex("\\*.+(?=\\s)");
    this->MessageBody = std::regex_replace(this->ResponseString, startAndEndRegex, "");
    // this->MessageBody = match[2];
    // Removing remaining part of fetch response (FLAGS)
    /*std::regex removeFlagsRegex("([\\s\\S]+)(?=\n)");*/
    /*std::regex_search(this->MessageBody, match, removeFlagsRegex);*/
    /*this->MessageBody = match[0];*/
    // Removing newline from the beggining of the message
    /*std::regex leadingNewLine("\n([\\s\\S]+)");*/
    /*std::regex_search(this->MessageBody, match, leadingNewLine);*/
    /*this->MessageBody = match[1];*/
    this->MessageBody = this->MessageBody.substr(2, this->RfcSize);
}

void Message::DumpToFile(const std::string &outDirectoryPath)
{
    std::ofstream file(outDirectoryPath + "/" + this->FileName);
    file << this->MessageBody;
    file.close();
}
