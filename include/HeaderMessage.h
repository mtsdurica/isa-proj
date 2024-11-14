/**
 * @file HeaderMessage.h
 * @author  Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Contains declaration of HeaderMessage class
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "Message.h"
#include <string>
class HeaderMessage final : public Message
{
  public:
    HeaderMessage(const std::string &messageUID, const std::string &responseString);
    ~HeaderMessage();
    /**
     * @brief Parse the filename from the message body
     *
     * @param serverHostname Remote server hostname
     * @param mailbox Remote mailbox from which the mail was fetched
     */
    void ParseFileName(const std::string &serverHostname, const std::string &mailbox);
    /**
     * @brief Parse message body to comply with RFC5322
     *
     */
    void ParseMessageBody();
};
