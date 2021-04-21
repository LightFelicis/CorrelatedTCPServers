#ifndef SIK_CONNECTIONHANDLER_H
#define SIK_CONNECTIONHANDLER_H
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

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
        exit_on_fail(tokens.size() == 3, "Bad servers file format");
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
        exit_on_fail(!is.failbit, "Cannot read file " + correlatedServersFile);
    }

    void handleIncomingConnection(int socket) {
        bool hasClosedConnection = false;
        std::string remainingCharacters = "";
        bool clrfBefore = false;
        bool isOffsetInBuffer = false;
        std::vector<std::string> httpRequestTokens;

        while (!hasClosedConnection) {
            char buffer[4096];
            int len = read(socket, buffer+isOffsetInBuffer, sizeof(buffer) - isOffsetInBuffer);
            exit_on_fail_with_errno(len >= 0, "Read() failed.");
            // Iterate over the buffer and split by \r\n characters
            int i = 0;
            for (i = 0; i < len - 1; i++) {
                if (!(buffer[i] == '\r' && buffer[i+1] == '\n')) {
                    remainingCharacters += buffer[i];
                    clrfBefore = false;
                } else if (!clrfBefore) {
                    httpRequestTokens.push_back(remainingCharacters);
                    remainingCharacters.clear();
                    clrfBefore = true;
                    i++;
                } else {
                    if (handleSingleRequest(httpRequestTokens, socket)) {
                        hasClosedConnection = true;
                    }
                    httpRequestTokens.clear();
                    clrfBefore = false;
                    i++;
                }
            }
            if (i == len - 1) { // Additional character unused
                buffer[0] = buffer[i];
                isOffsetInBuffer = true;
            } else {
                isOffsetInBuffer = false;
            }

        }
    }
private:
    std::vector<CorrelatedFile> correlatedFiles;
    std::string filesDir;
    bool handleSingleRequest(std::vector<std::string> tokens, int socket) {
        std::optional<HttpMessage> validated = HttpMessage::validateHttpRequest(tokens);
        if (!validated) {
            sendError(socket, "Bad syntax", "400");
            return true;
        }
        auto validatedMessage = validated.value();
        if (validatedMessage.getStartLine().getMethod() == "GET") {

        } else if (validatedMessage.getStartLine().getMethod() == "HEAD"){

        } else {
            sendError(socket, "Not implemented", "501");
            return true;
        }

    }

    void sendError(int socket, const std::string &reasoning, const std::string &statusCode) {
        std::string statusLine = HttpMessage::generateResponseStatusLine(statusCode, reasoning);
        statusLine += "\r\nConnection: close\r\n\r\n";
        exit_on_fail_with_errno(write(socket, statusLine.c_str(), statusLine.size()) >= 0, "Write() failed.");
    }

    void handleGetOrHeadRequest(const HttpMessage &hm, int socket, bool writeContent) {
        std::uintmax_t fileSize = 0;
        std::string contentType = "application/octet-stream";
        if (!std::filesystem::exists(hm.getStartLine().getRequestTarget())) { // Search in correlated files list.

        } else {
            fileSize = std::filesystem::file_size(hm.getStartLine().getRequestTarget());
            exit_on_fail(fileSize != static_cast<std::uintmax_t>(-1), "Checking the size of file failed()");
            if (writeContent) {
                std::ifstream is(hm.getStartLine().getRequestTarget());
                
            }
        }
    }


};


#endif //SIK_CONNECTIONHANDLER_H
