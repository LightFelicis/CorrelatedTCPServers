#ifndef ZALICZENIOWE1_CONNECTIONHANDLER_H
#define ZALICZENIOWE1_CONNECTIONHANDLER_H

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <unistd.h>

#include "../utils/serverAssertions.h"
#include "../utils/httpParsers.h"
#include "../utils/pathUtils.h"

struct CorrelatedFile {
    CorrelatedFile(const std::string &s) {
        std::istringstream ss(s);
        std::string substr;
        std::vector<std::string> tokens;
        while (getline(ss, substr, '\t')) {
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

    std::string toString() const {
        return "http://" + ipAddress + ":" + port + resource;
    }

    std::string resource;
    std::string ipAddress;
    std::string port;
};

class ConnectionHandler {
public:
    ConnectionHandler(const std::string &filesDirectory, const std::string &correlatedServersFile) : filesDir(
            filesDirectory) {
        std::ifstream is(correlatedServersFile);
        std::string line;

        while (std::getline(is, line)) {
            correlatedFiles.emplace_back(line);
        }
        exit_on_fail(is.eof(), "Cannot read file " + correlatedServersFile);
    }

    void handleIncomingConnection(int socket) {
        bool hasClosedConnection = false;
        std::string remainingCharacters = "";
        bool clrfBefore = false;
        bool isOffsetInBuffer = false;
        std::vector<std::string> httpRequestTokens;
        char buffer[4096];
        while (!hasClosedConnection) {
            int len = read(socket, buffer + isOffsetInBuffer, sizeof(buffer) - isOffsetInBuffer);
            exit_on_fail_with_errno(len >= 0, "Read() failed.");
            // Iterate over the buffer and split by \r\n characters
            int i;
            for (i = 0; i < len - 1 + isOffsetInBuffer; i++) {
                if (!(buffer[i] == '\r' && buffer[i + 1] == '\n')) {
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
        bool closeConnection;

        std::optional<HttpMessage> validated = HttpMessage::validateHttpRequest(tokens);
        if (!validated) {
            sendError(socket, "Bad syntax", "400");
            return true;
        }
        if (!validated.value().getStartLine().validCharacters()) {
            closeConnection = sendError(socket, "Not found", "404");
            return closeConnection;
        }
        auto validatedMessage = validated.value();
        const std::string method = validatedMessage.getStartLine().getMethod();
        if (method == "GET" || method == "HEAD") {
            closeConnection = handleGetOrHeadRequest(validatedMessage, socket, method == "GET");
        } else {
            sendError(socket, "Not implemented", "501");
            return true;
        }
        // Check if "Connection: close" is in the headers.
        for (const HeaderField &header : validatedMessage.getHeaderFields()) {
            if (header.getName() == "connection" && header.getValue() == "close") {
                closeConnection = true;
            }
        }
        return closeConnection;
    }

    bool sendError(int socket, const std::string &reasoning, const std::string &statusCode) {
        std::string statusLine = HttpMessage::generateResponseStatusLine(statusCode, reasoning);
        std::string toSend;
        if (statusCode != "404") {
            toSend = HttpMessage::generateHttpString({statusLine,
                                                      "Connection: close",
                                                     });
        } else {
            toSend = HttpMessage::generateHttpString({statusLine});
        }

        long unsigned int bytesWritten = write(socket, toSend.c_str(), toSend.size());
        return bytesWritten != toSend.size();
    }

    bool sendOctetStream(int socket, std::streamsize bytesToSend) {
        std::string statusLine = HttpMessage::generateResponseStatusLine("200", "OK");
        std::string toSend = HttpMessage::generateHttpString({statusLine,
                                                              "Content-Type: application/octet-stream",
                                                              "Content-Length: " + std::to_string(bytesToSend)
                                                             });
        long unsigned int bytesWritten = write(socket, toSend.c_str(), toSend.size());
        return bytesWritten != toSend.size();
    }

    bool sendRedirectToCorrelatedServer(int socket, const CorrelatedFile &cf) {
        std::string statusLine = HttpMessage::generateResponseStatusLine("302", "Redirected");
        std::string toSend = HttpMessage::generateHttpString({statusLine,
                                                              "Location: " + cf.toString(),
                                                             });
        long unsigned int bytesWritten = write(socket, toSend.c_str(), toSend.size());
        return bytesWritten != toSend.size();
    }

    bool handleGetOrHeadRequest(const HttpMessage &hm, int socket, bool writeContent) {
        const std::string requestTarget = hm.getStartLine().getRequestTarget();
        std::string filename = requestTarget;
        if (!validatePath(filesDir, filename)) {
            return sendError(socket, "Not found", "404");
        }
        filename = filesDir + filename;
        if (!std::filesystem::exists(filename)) { // Search in correlated files list.
            for (const CorrelatedFile &f : correlatedFiles) {
                if (f.resource == requestTarget) {
                    return sendRedirectToCorrelatedServer(socket, f);
                }
            }
            return sendError(socket, "Not found", "404");
        } else {
            if (!std::filesystem::is_regular_file(filename)) {
                return sendError(socket, "Not found", "404");
            }
            std::ifstream is(filename, std::ios::in | std::ios::binary);
            std::uintmax_t fileSize = std::filesystem::file_size(filename);
            bool isOk = sendOctetStream(socket, fileSize);
            std::vector<char> buffer(4096);
            if (writeContent) {
                do {
                    is.read(buffer.data(), 4096);
                    std::streamsize bytesRead = is.gcount();
                    int bytesSent = write(socket, buffer.data(), bytesRead);
                    isOk = isOk | (bytesSent != bytesRead);
                } while (is.good());
                exit_on_fail(is.eof(), "Reading file has failed.");
            }
            return isOk;
        }
    }
};


#endif //ZALICZENIOWE1_CONNECTIONHANDLER_H
