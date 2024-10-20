#pragma once

#include "Message.h"
#include <string>
class HeaderMessage final : public Message
{
  public:
    HeaderMessage(const std::string &messageUID, const std::string &responseString);
    ~HeaderMessage();
    void ParseFileName(const std::string &serverHostname, const std::string &mailbox);
    void ParseMessageBody();
};
