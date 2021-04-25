#ifndef SIK_CONNECTIONHANDLER_H
#define SIK_CONNECTIONHANDLER_H
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "../utils/serverAssertions.h"
#include "../utils/httpParsers.h"
#include "../utils/pathUtils.h"

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

    std::string toString() const {
        return "http://" + ipAddress + ":" + port + resource;
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
        exit_on_fail(is.eof(), "Cannot read file " + correlatedServersFile);
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
        for (auto t : tokens) {
            std::cout << "{ " << t << "}\n";
        }
        std::optional<HttpMessage> validated = HttpMessage::validateHttpRequest(tokens);
        if (!validated) {
            std::cout << "Uwaga uwaga wypisuje 400 bad syntax\n";
            sendError(socket, "Bad syntax", "400");
            return true;
        }
        if (!validated.value().getStartLine().validCharacters()) {
            std::cout << "Uwaga uwaga wypisuje 404\n";
            sendError(socket, "Not found", "404");
            return false;
        }
        auto validatedMessage = validated.value();
        const std::string method = validatedMessage.getStartLine().getMethod();
        if (method == "GET" || method == "HEAD") {
            handleGetOrHeadRequest(validatedMessage, socket, method == "GET");
        } else {
            sendError(socket, "Not implemented", "501");
            return true;
        }
        // Check if "Connection: close" is in the headers.
        bool closeConnection = false;
        for (const HeaderField &header : validatedMessage.getHeaderFields()) {
            if (header.getName() == "connection" && header.getValue() == "close") {
                closeConnection = true;
            }
        }
        return closeConnection;
    }

    void sendError(int socket, const std::string &reasoning, const std::string &statusCode) {
        std::string statusLine = HttpMessage::generateResponseStatusLine(statusCode, reasoning);
        std::string toSend;
        if (statusCode != "404") {
            toSend = HttpMessage::generateHttpString({statusLine,
                                                      "Connection: close",
                                                      });
        } else {
            toSend = HttpMessage::generateHttpString({statusLine});
        }

        exit_on_fail_with_errno(write(socket, toSend.c_str(), toSend.size()) >= 0, "Write() failed.");
    }

    void sendOctetStream(int socket, std::streamsize bytesToSend) {
        std::string statusLine = HttpMessage::generateResponseStatusLine("200", "OK");
        std::string toSend = HttpMessage::generateHttpString({statusLine,
                                                "Content-Type: application/octet-stream",
                                                "Content-Length: " + std::to_string(bytesToSend)
                                                });
        exit_on_fail_with_errno(write(socket, toSend.c_str(), toSend.size()) >= 0, "Write() failed.");
    }

    void sendRedirectToCorrelatedServer(int socket, const CorrelatedFile &cf) {
        std::string statusLine = HttpMessage::generateResponseStatusLine("302", "Redirected");
        std::string toSend = HttpMessage::generateHttpString({statusLine,
                                                              "Location: " + cf.toString(),
                                                             });
        exit_on_fail_with_errno(write(socket, toSend.c_str(), toSend.size()) >= 0, "Write() failed.");
    }

    void handleGetOrHeadRequest(const HttpMessage &hm, int socket, bool writeContent) {
        std::string filename = hm.getStartLine().getRequestTarget();
        if (!validatePath(filesDir, filename)) {
            sendError(socket, "400", "Malicious path detected");
            return;
        }
        filename = filesDir + filename;
        if (!std::filesystem::is_regular_file(filename)) {
            sendError(socket, "Not found", "404");
            return;
        }

        if (!std::filesystem::exists(filename)) { // Search in correlated files list.
            std::cerr << "Nie ma takiego pliku, sprawdzam w correlated..." << std::endl;
            bool found = false;
            for (const CorrelatedFile &f : correlatedFiles) {
                if (f.resource == filename) {
                    found = true;
                    sendRedirectToCorrelatedServer(socket, f);
                    break;
                }
            }

            if (!found) {
                sendError(socket, "Not found", "404");
            }
        } else {
            std::ifstream is(filename, std::ios::in | std::ios::binary);
            std::uintmax_t fileSize = std::filesystem::file_size(filename);
            sendOctetStream(socket, fileSize);
            std::vector<char> buffer(4096);
            if (writeContent) {
                do {
                    is.read(buffer.data(), 4096);
                    std::streamsize bytesRead = is.gcount();
                    exit_on_fail_with_errno(write(socket, buffer.data(), bytesRead) >= 0, "Write() failed.");
                } while (is.good());
                exit_on_fail(is.eof(), "Reading file has failed.");
            }
        }
    }
};


#endif //SIK_CONNECTIONHANDLER_H
