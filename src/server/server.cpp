/*
 * Base server implementation.
 * Server establishes TCP connection with at most one client at the time.
 * Usage: ./server directory_with_files correlated_servers_file [port_num]
 * where default value of port_num is 8080
 * */
#include <inttypes.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <filesystem>
#include <unistd.h>

#include "../utils/serverAssertions.h"
#include "connectionHandler.h"

void startServer(uint16_t portnum, const std::string &filesDirectory, const std::string &correlatedServersFile) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in client_address;
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portnum);

    ConnectionHandler ch(filesDirectory, correlatedServersFile);
    exit_on_fail(bind(sockfd, (sockaddr *)&address, sizeof(address)) >= 0, "Bind() error.");
    while (true) {
        socklen_t client_address_len = sizeof(client_address);
        // get client connection from the socket
        int msg_sock = accept(sockfd, (struct sockaddr *) &client_address, &client_address_len);
        exit_on_fail(msg_sock >= 0, "Accept() error.");
        ch.handleIncomingConnection(msg_sock);
        exit_on_fail(close(msg_sock) >= 0, "Close(socket) failed.")
    }
}

int main(int argc, char **argv) {
    exit_on_fail(argc >= 3, "Usage: ./server directory_with_files correlated_servers_file [port_num]");

    uint16_t port_num = 8080;
    if (argc == 4) {
        try {
            port_num = std::stoi(argv[3]);
        } catch (std::invalid_argument e) {
            exit_on_fail(false, "Wrong port num");
        }
    }
    exit_on_fail(std::filesystem::exists(argv[1]), "Server's files directory doesn't exists.");
    exit_on_fail(std::filesystem::exists(argv[2]), "Correlated servers file doesn't exists.");
    startServer(port_num, argv[1], argv[2]);
}
