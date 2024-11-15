/**
 * @file Message.h
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains declaration of Message class
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include <string>

class Message
{
  protected:
    std::string MessageUID;
    std::string ResponseString;
    std::string FileName;
    std::string MessageBody;
    int RfcSize;

  public:
    Message();
    Message(const std::string &messageUID, const std::string &responseString, int rfcSize);
    virtual ~Message();
    /**
     * @brief Parse the filename from the message body
     *
     * @param serverHostname Remote server hostname
     * @param mailbox Remote mailbox from which the mail was fetched
     */
    virtual void ParseFileName(const std::string &serverHostname, const std::string &mailbox);
    /**
     * @brief Parse message body to comply with RFC5322
     *
     */
    virtual void ParseMessageBody();
    /**
     * @brief Dump message body to a local file
     *
     * @param outDirectoryPath Path to the output directory
     */
    virtual void DumpToFile(const std::string &outDirectoryPath);
};
