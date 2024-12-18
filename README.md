# imapcl

A command-line IMAP client

## Author

Matúš Ďurica (xduric06)

## Date

16.11.2024

## Items submitted

-   include/
-   src/
-   tests/
-   Makefile
-   manual.pdf
-   README.md

## Introduction

This application allows user to read email using the `IMAPrev1 (RFC 3501)` protocol. Each message is downloaded and saved separately in the `Internet Message Format (RFC 5322)` format to a directory specified by the user. Its output is a message which tells the user how many messages were fetched and stored. The user can tweak the functionality further using command-line arguments.

## Usage

```utf-8
./imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a auth_file [-b MAILBOX] -o out_dir
```

```utf-8
server          - Required IP address/hostname of an IMAP server
-p port         - Optional port number
                  DEFAULT VALUE:
                  - 143 for unencrypted communication
                  - 993 for encrypted communication
-T              - Turns on encrypted communication (imaps)
-c certfile     - Optional certificate file, used for verifying SSL/TLS certificate from the server
-C certaddr     - Optional certificate directory, where certificates for verifying the SSL/TLS certificate from the server will be looked up
-n              - Only new messages will be fetched
-h              - Only headers will be fetched
-a auth_file    - Required path to a file containing username and password for authentication on the server
-b MAILBOX      - Optional mailbox name
                  DEFAULT VALUE:
                  - INBOX
-o out_dir      - Required path to a directory to which messages will be fetched
```

## Building the executable

-   GNU make

### Prerequisities

-   GNU make
-   g++
-   netinet/\*
-   sys/\*
-   arpa/\*
-   openssl/\*
-   libcrypto

### Compiling

Application can be build using following commands from the root directory of the project:

```utf-8
make
```

or

```utf-8
make all
```

or it can be built to display debug information

```utf-8
make debug
```

## Testing

Automated unit tests for testing the parsing of arguments and the authentication file are provided with the project. Integration testing was done manually with `zoznam.sk` dummy account and with my faculty email account.

Wireshark was also used to check the unencrypted communication.

### Prerequsites

-   GNU make

-   GTest

Automated tests are provided in this project and can be run from the project's root directory using:

```utf-8
make test
```

### Automated tests output

```utf-8
[==========] Running 22 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 6 tests from Arguments
[ RUN      ] Arguments.MissingServer
[2] ERROR: Missing server address
[       OK ] Arguments.MissingServer (0 ms)
[ RUN      ] Arguments.MissingAuthFile
[3] ERROR: Missing required argument
[       OK ] Arguments.MissingAuthFile (0 ms)
[ RUN      ] Arguments.MissingOutDirectory
[3] ERROR: Missing required argument
[       OK ] Arguments.MissingOutDirectory (0 ms)
[ RUN      ] Arguments.BadAuthFile
[12] ERROR: Auth file does not exist
[       OK ] Arguments.BadAuthFile (0 ms)
[ RUN      ] Arguments.BadOutputDirectory
[15] ERROR: Output directory does not exist
[       OK ] Arguments.BadOutputDirectory (0 ms)
[ RUN      ] Arguments.UnknownArgument
[5] ERROR: Unknown argument
[       OK ] Arguments.UnknownArgument (0 ms)
[----------] 6 tests from Arguments (0 ms total)

[----------] 12 tests from ArgumentsMissingOptions
[ RUN      ] ArgumentsMissingOptions.MissingOptionAuthFile
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionAuthFile (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionInbox
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionInbox (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionCertFile
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionCertFile (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionCertDir
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionCertDir (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionOutDir
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionOutDir (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionPort
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionPort (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionAuthFileEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionAuthFileEnd (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionInboxEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionInboxEnd (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionCertFileEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionCertFileEnd (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionCertDirEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionCertDirEnd (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionOutDirEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionOutDirEnd (0 ms)
[ RUN      ] ArgumentsMissingOptions.MissingOptionPortEnd
[4] ERROR: Missing required argument option
[       OK ] ArgumentsMissingOptions.MissingOptionPortEnd (0 ms)
[----------] 12 tests from ArgumentsMissingOptions (0 ms total)

[----------] 4 tests from AuthenticationFile
[ RUN      ] AuthenticationFile.MissingUsername
[14] ERROR: Missing username or invalid username format
[       OK ] AuthenticationFile.MissingUsername (0 ms)
[ RUN      ] AuthenticationFile.SpacesInUsername
[       OK ] AuthenticationFile.SpacesInUsername (0 ms)
[ RUN      ] AuthenticationFile.MissingPassword
[14] ERROR: Missing password or invalid password format
[       OK ] AuthenticationFile.MissingPassword (0 ms)
[ RUN      ] AuthenticationFile.SpacesInPassword
[       OK ] AuthenticationFile.SpacesInPassword (0 ms)
[----------] 4 tests from AuthenticationFile (2 ms total)

[----------] Global test environment tear-down
[==========] 22 tests from 3 test suites ran. (2 ms total)
[  PASSED  ] 22 tests.
```

## Implementation details

### File names

#### Full message file name

Names of saved messages follow follow this convention:

```utf-8
<UID>_<Mailbox>_<Hostname>_<Subject>_<Sender>_<Hash>.eml
```

#### Header file name

Names of saved headers follow the same convention with added `_h` before the file extension.

### Authentication file

Authentication file is used to store username and password.

Authentication file has to follow this format:

```utf-8
username = <username>
password = <password>
```

## Limitations

During the testing on the faculty email with the `-n` argument, I discovered that the faculty's IMAP server `imap.stud.fit.vutbr.cz` was not searching NEW (messages that have the `\Recent` flag set but not the `\Seen` flag) messages.

I confirmed this by connecting to the faculty IMAP server using the `openssl` command-line tool and searching for NEW messages manually (of course, with newly received messages that could not have had their `\Recent` flag removed by a previous session). Cross-checking with my dummy email on zoznam.sk resulted in the application working as expected.

Therefore, I believe this has to do something with the faculty IMAP server configuration because no other instances of any IMAP clients (i.e. Thunderbird, Outlook) were running on my computer in the background. I also tried running the application on the faculty's `merlin.fit.vutbr.cz` server to no avail.

Because searching for `NEW` messages worked on my dummy `zoznam.sk` account, I decided to keep searching for `NEW` messages instead of searching for `UNSEEN` messages.

Due to no closer specification of the username and password format, the implementation allows spaces between strings. This means that `username = example example` would be evaluated as string `example example`, which in itself is not incorrect, but when authenticating on the IMAP server, this would result in too many arguments in the `LOGIN` command, which would be interpreted as incorrect by the IMAP server, resulting in the application reporting invalid response from the server, rather than that the authentication failed due to incorrect credentials, which might be confusing for the user.
