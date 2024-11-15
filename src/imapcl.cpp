/**
 * @file imapcl.cpp
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Program entry point
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "../include/EncryptedSession.h"
#include "../include/Session.h"
#include "../include/Utils.h"

int main(int argc, char **argv)
{
    Utils::Arguments arguments;
    Utils::ReturnCodes returnCode;
    if ((returnCode = Utils::CheckArguments(argc, argv, arguments)))
        return returnCode;
    std::unique_ptr<Session> session;
    if (arguments.Encrypted)
        session = std::make_unique<EncryptedSession>(arguments.ServerAddress, arguments.Port, arguments.Username,
                                                     arguments.Password, arguments.OutDirectoryPath, arguments.MailBox,
                                                     arguments.CertificateFile, arguments.CertificateFileDirectoryPath);
    else
        session = std::make_unique<Session>(arguments.ServerAddress, arguments.Port, arguments.Username,
                                            arguments.Password, arguments.OutDirectoryPath, arguments.MailBox);

    if ((returnCode = session->GetHostAddressInfo()))
        return returnCode;
    if ((returnCode = session->CreateSocket()))
        return returnCode;
    if ((returnCode = session->Connect()))
        return returnCode;
    if ((returnCode = session->Authenticate()))
        return returnCode;
    if ((returnCode = session->FetchMail(arguments.OnlyMailHeaders, arguments.OnlyNewMails)))
        return returnCode;
    if ((returnCode = session->Logout()))
        return returnCode;

    return Utils::IMAPCL_SUCCESS;
}
