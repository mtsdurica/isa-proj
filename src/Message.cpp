#include "../include/Message.h"

#include <fstream>
#include <iostream>
#include <regex>

Message::Message(const std::string &messageUID, const std::string &responseString)
    : MessageUID(messageUID), ResponseString(responseString), FileName(""), MessageBody("")
{
}

Message::~Message() = default;

void Message::ParseFileName()
{

    this->FileName = this->MessageUID + "_";
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
    /*std::cerr << this->FileName << "\n";*/
}

void Message::ParseMessageBody()
{
    // Removing start and end of the fetch response
    std::regex startAndEndRegex(
        "(\\*\\s[0-9]+\\sFETCH\\s*\\(BODY\\[\\]\\s\\{[0-9]+\\}\\s)([\\s\\S]+)(\\sUID\\s[0-9]+\\))");
    std::smatch match;
    std::regex_search(this->ResponseString, match, startAndEndRegex);
    this->MessageBody = match[2];
    // Removing remaining part of fetch response (FLAGS)
    std::regex removeFlagsRegex("([\\s\\S]+)(?=\n)");
    std::regex_search(this->MessageBody, match, removeFlagsRegex);
    this->MessageBody = match[0];
    // Removing newline from the beggining of the message
    std::regex leadingNewLine("\n([\\s\\S]+)");
    std::regex_search(this->MessageBody, match, leadingNewLine);
    this->MessageBody = match[1];
}

void Message::DumpToFile(const std::string &outDirectoryPath)
{
    std::ofstream file(outDirectoryPath + "/" + this->FileName);
    file << this->MessageBody;
    file.close();
}
