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
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/Communication.h"
#include "../include/Utils.h"

int main(int argc, char **argv)
{
    /**
     * TODO:
     *
     * Check current tag + OK in every response
     * Data transmitted by the server to the client and status responses
     * that do not indicate command completion are prefixed with the token
     * "*", and are called untagged responses.
     *
     * Retrieve UIDVality, store it and everytime you connect after that,
     * check retrieved UIDV against the stored one
     * If it did not change, check if the mail is downloaded, if not download it
     * If it did, remove all mail and download everything again
     *
     * -n
     * TAG SELECT MAILBOX
     * TAG SEARCH UNSEEN -> extract UIDs of unseen messages
     * TAG FETCH one by one from numbers extracted above
     *
     * TAG SELECT MAILBOX
     * TAG SEARCH ALL -> extract UIDs of all messages
     * compare which are downloaded locally and which are not
     * TAG FETCH one by one from numbers which are not local
     */
    Utils::Arguments arguments;
    Utils::ReturnCodes returnCode = Utils::CheckArguments(argc, argv, arguments);
    if (returnCode)
        return returnCode;
    Communication communication(arguments.Username, arguments.Password);
    if (communication.GetHostAddressInfo(arguments.ServerAddress, arguments.Port))
        return Utils::SERVER_BAD_HOST;
    if (communication.CreateSocket())
        return Utils::SOCKET_CREATING;
    if (communication.Connect())
        return Utils::SOCKET_CONNECTING;
    if (communication.Authenticate())
        return Utils::AUTH_FILE_OPEN;
    if (communication.Logout())
        return Utils::AUTH_FILE_OPEN;
    return Utils::IMAPCL_SUCCESS;
}
