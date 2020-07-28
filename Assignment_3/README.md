# Network Programming Assignment 3

This folder contains my solutions for assignment 3 of on-campus Network Programming (IS F462) course. The file description is as follows:

1. `182_NP_assignment-3.pdf`: It describes the problem statement.
2. `inetd.c`: It contains code for the Inetd server program.
3. `echo.c`: It contains code for the echo TCP server program.
4. `echoudp.c`: It contains code for the echo UDP server program.
5. `echoclient.c`: It contains code for the echo TCP client program.
6. `echoudpclient.c`: It contains code for the echo UDP client program.
7. `my_inetd.conf`: It is the local configuration file for the inetd server.
8. `makefile`: It compiles the code to executable files `inetdserver`, `tcpclient`, `udpclient`, `echoserver` and `echoudpserver`.
9. `Assignment3_Design_Doc.pdf`: It describes the various design choices made while implementing the Inetd server.

## Steps To Run The Code:
This code is implemented in `C` language. To compile the code, use the following command:
```sh
make
``` 
To run the Inetd server, run:
```sh
./inetdserver
```
To run TCP client, run:
```sh
./tcpclient
```
To run UDP client, run:
```sh
./udpclient
```

## Introduction/Problem Statement:

The given problem asks to implement an inetd service. Inetd is a super server, which runs multiple servers as specified in an inetd.conf file. You have to implement both wait and no-wait modes for the inetd server.

