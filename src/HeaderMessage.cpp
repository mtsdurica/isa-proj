/**
 * @file HeaderMessage.cpp
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains definitions of HeaderMessage class methods
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "../include/HeaderMessage.h"

#include <iostream>
#include <regex>

HeaderMessage::HeaderMessage(const std::string &messageUID, const std::string &responseString)
    : Message(messageUID, responseString, -1)
{
}

HeaderMessage::~HeaderMessage() = default;

void HeaderMessage::ParseFileName(const std::string &serverHostname, const std::string &mailbox)
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
    this->FileName += "_h.eml";
}

void HeaderMessage::ParseMessageBody()
{
    // Removing start and end of the fetch response
    std::regex headerSizeRegex("\\*.+\\{([0-9]+)(?=\\}\\s)");
    std::smatch headerSizeMatch;
    std::regex_search(this->ResponseString, headerSizeMatch, headerSizeRegex);
    int headerSize = stoi(headerSizeMatch[1]);
    std::regex firstLineRemoveRegex("\\*.+(?=\\s)");
    this->MessageBody = std::regex_replace(this->ResponseString, firstLineRemoveRegex, "");
    this->MessageBody = this->MessageBody.substr(2, headerSize);
}
