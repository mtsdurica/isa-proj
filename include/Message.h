#pragma once
#include <string>

class Message final
{
  protected:
    std::string MessageUID;
    std::string ResponseString;
    std::string FileName;
    std::string MessageBody;

  public:
    Message(const std::string &messageUID, const std::string &responseString);
    ~Message();
    void ParseFileName();
    void ParseMessageBody();
    void DumpToFile(const std::string &outDirectoryPath);
};
