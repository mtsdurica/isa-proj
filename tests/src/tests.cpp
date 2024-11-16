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

#include "../../include/Session.h"
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

TEST(Arguments, BadOutputDirectory)
{
    int numOfArguments = 6;
    char *args[] = {(char *)"./imapcl",
                    (char *)"example.server",
                    (char *)"-a",
                    (char *)"./tests/resources/example",
                    (char *)"-o",
                    (char *)"./tests/resources/example/",
                    nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::OUT_DIR_NONEXISTENT, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(Arguments, UnknownArgument)
{
    int numOfArguments = 7;
    char *args[] = {(char *)"./imapcl", (char *)"example.server",
                    (char *)"-a",       (char *)"./tests/resources/example",
                    (char *)"-o",       (char *)"./tests/resources/example/",
                    (char *)"-x",       nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_UNKNOWN_ARGUMENT, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionAuthFile)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl",
                    (char *)"example.server",
                    (char *)"-a",
                    (char *)"-o",
                    (char *)"./tests/resources/example/",
                    nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionInbox)
{
    int numOfArguments = 7;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-b", (char *)"-a", (char *)"./tests/resources/example/",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionCertFile)
{
    int numOfArguments = 7;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-c", (char *)"-a", (char *)"./tests/resources/example/",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionCertDir)
{
    int numOfArguments = 7;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-C", (char *)"-a", (char *)"./tests/resources/example/",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionOutDir)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl",
                    (char *)"example.server",
                    (char *)"-o",
                    (char *)"-a",
                    (char *)"./tests/resources/example/",
                    nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionPort)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl",
                    (char *)"example.server",
                    (char *)"-p",
                    (char *)"-a",
                    (char *)"./tests/resources/example/",
                    nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionAuthFileEnd)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl", (char *)"example.server",
                    (char *)"-o",       (char *)"./tests/resources/example/",
                    (char *)"-a",       nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionInboxEnd)
{
    int numOfArguments = 7;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/example/",
                    (char *)"-o",       (char *)"/dev/null",      (char *)"-b", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionCertFileEnd)
{
    int numOfArguments = 7;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/example/",
                    (char *)"-o",       (char *)"/dev/null",      (char *)"-c", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionCertDirEnd)
{
    int numOfArguments = 7;
    char *args[] = {(char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/example/",
                    (char *)"-o",       (char *)"/dev/null",      (char *)"-C", nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionOutDirEnd)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl", (char *)"example.server",
                    (char *)"-a",       (char *)"./tests/resources/example/",
                    (char *)"-o",       nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(ArgumentsMissingOptions, MissingOptionPortEnd)
{
    int numOfArguments = 5;
    char *args[] = {(char *)"./imapcl", (char *)"example.server",
                    (char *)"-a",       (char *)"./tests/resources/example/",
                    (char *)"-p",       nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_OPTION, Utils::CheckArguments(numOfArguments, args, arguments));
}

TEST(AuthenticationFile, MissingUsername)
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

TEST(AuthenticationFile, SpacesInUsername)
{
    int numOfArguments = 6;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/authSpacesInUName.txt",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::IMAPCL_SUCCESS, Utils::CheckArguments(numOfArguments, args, arguments));
    ASSERT_EQ("test test", arguments.Username);
}

TEST(AuthenticationFile, MissingPassword)
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

TEST(AuthenticationFile, SpacesInPassword)
{
    int numOfArguments = 6;
    char *args[] = {
        (char *)"./imapcl", (char *)"example.server", (char *)"-a", (char *)"./tests/resources/authSpacesInPWord.txt",
        (char *)"-o",       (char *)"/dev/null",      nullptr};
    // Reset optind before each test run
    optind = 1;
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::IMAPCL_SUCCESS, Utils::CheckArguments(numOfArguments, args, arguments));
    ASSERT_EQ("test test", arguments.Password);
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
