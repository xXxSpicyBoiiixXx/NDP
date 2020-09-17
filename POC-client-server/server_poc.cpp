/*
# FILE: server_poc.cpp
# USAGE: --
# DESCRIPTION: POC for a server written in C++.
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

using namespace std;

int main()
{
    int client,
        server;
    int port_number = 4444;
    bool exit_func = false;
    int buffer_size = 1024;
    socklen_t size;
    char buffer[buffer_size];
    socklen_t size;

    struct sockaddr_in server_addr;

    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0)
    {
        cout << "\nThe socket cannot be established..." << endl;
        exit(1);
    }

    cout << "\nSocket server has been created..." << endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(port_number);

    if ((bind(client, (struct sockaddr*)&server_addr,sizeof(server_addr))) < 0)
    {
        cout << "Error binding connection, the socket has already been established..." << endl;
        return -1;
    }

    size = sizeof(server_addr);
    cout << "Looking for clients..." << endl;

    listen(client, 1);
    int clientCount = 1;
    server = accept(client,(struct sockaddr *)&server_addr,&size);

    if (server < 0)
        cout << "Error on accepting..." << endl;

    while (server > 0)
    {
        strcpy(buffer, "Server connected...\n");
        send(server, buffer, buffer_size, 0);
        cout << "Connected with the client #" << clientCount << ", good to go fam..." << endl;
        cout << "\n Enter # to end the connection\n" << endl;
        cout << "Client: ";
        do {
            recv(server, buffer, buffer_size, 0);
            cout << buffer << " ";
            if (*buffer == '#') {
                *buffer = '*';
                exit_func = true;
            }
        } while (*buffer != '*');

        do {
            cout << "\nServer: ";
            do {
                cin >> buffer;
                send(server, buffer, buffer_size, 0);
                if (*buffer == '#') {
                    send(server, buffer, buffer_size, 0);
                    *buffer = '*';
                    exit_func = true;
                }
            } while (*buffer != '*');

            cout << "Client: ";
            do {
                recv(server, buffer, buffer_size, 0);
                cout << buffer << " ";
                if (*buffer == '#') {
                    *buffer == '*';
                    exit_func = true;
                }
            } while (*buffer != '*');
        } while (!exit_func);

        cout << "\n\nConnection terminated with client " << inet_ntoa(server_addr.sin_addr);
        close(server);
        cout << "\nLater..." << endl;
        exit_func = false;
        exit(1);
    }

    close(client);
    return 0;
}
