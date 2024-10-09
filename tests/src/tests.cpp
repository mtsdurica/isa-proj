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
    char *argument = (char *)malloc(sizeof("./imapcl"));
    char *argv[1] = {argument};
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_SERVER, Utils::CheckArguments(numOfArguments, argv, arguments));
    free(argument);
}

TEST(Arguments, MissingAuthFile)
{
    int numOfArguments = 4;
    char *argument1 = (char *)malloc(sizeof("./imapcl"));
    char *argument2 = (char *)malloc(sizeof("example.server"));
    char *argument3 = (char *)malloc(sizeof("-o"));
    char *argument4 = (char *)malloc(sizeof("exampleDir"));
    char *argv[4] = {argument1, argument2, argument3, argument4};
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_REQUIRED, Utils::CheckArguments(numOfArguments, argv, arguments));
    free(argument1);
    free(argument2);
    free(argument3);
    free(argument4);
}

TEST(Arguments, MissingOutDirectory)
{
    int numOfArguments = 4;
    char *argument1 = (char *)malloc(sizeof("./imapcl"));
    char *argument2 = (char *)malloc(sizeof("example.server"));
    char *argument3 = (char *)malloc(sizeof("-a"));
    char *argument4 = (char *)malloc(sizeof("authFile"));
    char *argv[4] = {argument1, argument2, argument3, argument4};
    Utils::Arguments arguments;
    ASSERT_EQ(Utils::ARGS_MISSING_REQUIRED, Utils::CheckArguments(numOfArguments, argv, arguments));
    free(argument1);
    free(argument2);
    free(argument3);
    free(argument4);
}

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
