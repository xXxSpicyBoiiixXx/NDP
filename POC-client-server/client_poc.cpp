/*
# FILE: client_poc.cpp
# USAGE: --
# DESCRIPTION: POC for a client written in C++.
# OPTIONS: --
# REQUIREMENTS: --
# BUGS: --
# AUTHOR: xXxSpicyBoiiixXx
# ORGANIZATION: HExSA Lab (IIT)
# VERSION: 1.0
# CREATED: 09/16/2020
# REVISION: --
*/

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

int main()
{
    int client;
    int portNum = 4444;
    bool isExit = false;
    int buffsize = 1024;
    char buffer[buffsize];
    char* ip = "127.0.0.1";

    struct sockaddr_in server_addr;

    client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0)
    {
        cout << "\nThe socket cannot be established..." << endl;
        exit(1);
    }

    cout << "\nSocket client has been created..." << endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);

    if (connect(client,(struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
        cout << "Connection to the server port number: " << portNum << endl;

    cout << "Awaiting connection from the server..." << endl; //line 40
    recv(client, buffer, buffsize, 0);
    cout << "Connection confirmed, good to go...";

    cout << "\n\nEnter # to end the connection\n" << endl;

    // Here, the client can send a message first.
    do {
        cout << "Client: ";
        do {
            cin >> buffer;
            send(client, buffer, buffsize, 0);
            if (*buffer == '#') {
                send(client, buffer, buffsize, 0);
                *buffer = '*';
                isExit = true;
            }
        } while (*buffer != 42);

        cout << "Server: ";
        do {
            recv(client, buffer, buffsize, 0);
            cout << buffer << " ";
            if (*buffer == '#') {
                *buffer = '*';
                isExit = true;
            }

        } while (*buffer != 42);
        cout << endl;

    } while (!isExit);

    cout << "\nConnection terminated.\n";
    close(client);
    return 0;
}
