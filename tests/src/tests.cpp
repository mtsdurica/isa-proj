/**
 * @file tests.cpp
 * @author Matúš Ďurica (xduric06@stud.fit.vutbr.cz)
 * @brief Unit tests for the imapcl application
 * @version 0.1
 * @date 2024-10-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdlib>
#include <gtest/gtest.h>

#include "../../include/Utils.h"

using namespace Utils;

TEST(Arguments, MissingServer)
{
    int numOfArguments = 1;
    char *args[] = {(char *)"./imapcl", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_SERVER, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, MissingAuthFile)
{
    int numOfArguments = 4;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-o", (char *)"example", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_REQUIRED, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, MissingOutDirectory)
{
    int numOfArguments = 4;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"example", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_REQUIRED, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, BadAuthFile)
{
    int numOfArguments = 6;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"authFile",
                    (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::AUTH_FILE_NONEXISTENT, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, MissingUsername)
{
    int numOfArguments = 6;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/authMissingUName.txt",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::AUTH_MISSING_CREDENTIALS, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, MissingPassword)
{
    int numOfArguments = 6;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/authMissingPWord.txt",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::AUTH_MISSING_CREDENTIALS, Utils::CheckArguments(numOfArguments, args, arguments));
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
