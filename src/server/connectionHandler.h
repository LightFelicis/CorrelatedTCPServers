#ifndef SIK_CONNECTIONHANDLER_H
#define SIK_CONNECTIONHANDLER_H
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "../utils/serverAssertions.h"
#include "../utils/httpParsers.h"

struct CorrelatedFile {
    CorrelatedFile(const std::string &s) {
        std::istringstream ss(s);
        std::string substr;
        std::vector<std::string> tokens;
        while(getline(ss, substr, '\t'))
        {
            tokens.push_back(substr);
        }
        resource = tokens[0];
        ipAddress = tokens[1];
        port = tokens[2];
    }

    bool operator==(const CorrelatedFile &cf) {
        return resource == cf.resource && ipAddress == cf.ipAddress && port == cf.port;
    }

    std::string resource;
    std::string ipAddress;
    std::string port;
};

class ConnectionHandler {
public:
    ConnectionHandler(const std::string &filesDirectory, const std::string &correlatedServersFile) : filesDir(filesDirectory){
        std::ifstream is(correlatedServersFile);
        std::string line;
        while (std::getline(is, line)) {
            correlatedFiles.emplace_back(line);
        }
    }

    void handleIncomingConnection(int socket) {
        bool hasClosedConnection = false;
        std::string remainingCharacters = "";
        bool clrfBefore = false;
        while (!hasClosedConnection) {
            std::vector<std::string> httpRequestTokens;
            char buffer[4096];
            int len = read(socket, buffer, sizeof(buffer));
            exit_on_fail(len >= 0, "Read() failed.");
            // Iterate over the buffer and split by \n\r characters
            for (int i = 0; i < len - 1; i++) {
                if (buffer[i] != '\n' && buffer[i+1] != '\r') {
                    remainingCharacters += buffer[i];
                    clrfBefore = false;
                } else if (clrfBefore) {
                    if (handleSingleRequest(httpRequestTokens, socket)) {
                        hasClosedConnection = true;
                    }
                } else {
                    i++;
                    httpRequestTokens.push_back(remainingCharacters);
                    remainingCharacters = "";
                    clrfBefore = true;
                }
            }
        }
    }
private:
    std::vector<CorrelatedFile> correlatedFiles;
    std::string filesDir;
    bool handleSingleRequest(std::vector<std::string> tokens, int socket) {
        auto validated = HttpMessage::validateHttpRequest(tokens);
        if (!validated) {
            send400Error(socket, "Bad syntax");
            return true;
        }
        // Tu ifowanie?
    }

    int send400Error(int socket, std::string reasoning) {
        std::string statusLine = HttpMessage::generateResponseStatusLine("400", reasoning);
        statusLine += "\n\rConnection: close\n\r\n\r";
        char *buffer = (char *)malloc(statusLine.size());
        for (int i = 0; i < statusLine.size(); i++) {
            buffer[i] = statusLine[i];
        }
        write(socket, buffer, statusLine.size());
        free(buffer);
    }
};


#endif //SIK_CONNECTIONHANDLER_H
