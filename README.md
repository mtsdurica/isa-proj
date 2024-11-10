# imapcl

A command-line IMAP client

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

### Compiling

Application can be build using following commands from the root directory of the project:

```utf-8
make
```

or

```utf-8
make all
```

## Testing

### Prerequsites

-   GNU make

-   GTest

Automated tests are provided in this project and can be run using:

```utf-8
make test
```

## ???

This application allows user to read email using the `IMAPrev1 (RFC 3501)` protocol. Each message is downloaded and saved separately in the `Internet Message Format (RFC 5322)` format to a directory specified by the user.

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
