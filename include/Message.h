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
    Message(const std::string &messageUID, const std::string &responseString, int rfcSize);
    ~Message();
    void ParseFileName(const std::string &serverHostname, const std::string &mailbox);
    void ParseMessageBody();
    void DumpToFile(const std::string &outDirectoryPath);
};
