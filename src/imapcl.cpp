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
    /**
     * TODO:
     * -n
     * TAG SELECT MAILBOX
     * TAG SEARCH UNSEEN -> extract UIDs of unseen messages
     * TAG FETCH one by one from numbers extracted above
     *
     * -h
     * -c -C
     *
     * Error handling
     */
    Utils::Arguments arguments;
    Utils::ReturnCodes returnCode = Utils::CheckArguments(argc, argv, arguments);
    if (returnCode)
        return returnCode;
    if (arguments.Encrypted)
    {
        EncryptedSession session(arguments.ServerAddress, arguments.Port, arguments.Username, arguments.Password,
                                 arguments.OutDirectoryPath, arguments.MailBox, arguments.CertificateFile,
                                 arguments.CertificateFileDirectoryPath);
        if (session.GetHostAddressInfo())
            return Utils::SERVER_BAD_HOST;
        if (session.CreateSocket())
            return Utils::SOCKET_CREATING;
        returnCode = session.Connect();
        if (returnCode)
            return returnCode;
        if (session.Authenticate())
            return Utils::AUTH_FILE_OPEN;
        if (session.FetchAllMail())
            return 1;
        if (session.Logout())
            return Utils::INVALID_RESPONSE;
    }
    else
    {
        Session session(arguments.ServerAddress, arguments.Port, arguments.Username, arguments.Password,
                        arguments.OutDirectoryPath, arguments.MailBox);

        if (session.GetHostAddressInfo())
            return Utils::SERVER_BAD_HOST;
        if (session.CreateSocket())
            return Utils::SOCKET_CREATING;
        if (session.Connect())
            return Utils::SOCKET_CONNECTING;
        if (session.Authenticate())
            return Utils::AUTH_FILE_OPEN;
        if (session.FetchAllMail())
            return 1;
        if (session.Logout())
            return Utils::INVALID_RESPONSE;
    }
    return Utils::IMAPCL_SUCCESS;
}
