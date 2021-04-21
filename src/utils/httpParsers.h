#ifndef ZALICZENIOWE1_PARSE_REQUEST_H
#define ZALICZENIOWE1_PARSE_REQUEST_H

#include <array>
#include <regex>
#include <optional>
#include <iostream>
#include <vector>

class StartLine {
public:
    static std::optional<StartLine> validateString(std::string s) {
        const std::regex startlineRegex(R"regex(([^\s]+) (/[a-zA-Z0-9.-/]+) (HTTP/1.1))regex");
        std::cmatch m;
        if (!std::regex_match(s.c_str(), m, startlineRegex)) {
            return {};
        }
        /*
         * Task description specifies that providing method other than GET/HEAD should be treated as a
         * "not implemented" error rather than "wrong argument" error, so StartLine stores
         * additional information if method is implemented.
         **/
        static const std::array<std::string, 2> implementedMethods = {"GET", "HEAD"};
        auto it = std::find(implementedMethods.begin(), implementedMethods.end(), m[1]);
        bool implemented = (it != implementedMethods.end());
        return StartLine(m[1], m[2], m[3], implemented);
    }

    std::string getMethod() const {
        return method;
    }

    std::string getRequestTarget() const {
        return requestTarget;
    }

    std::string getHttpVersion() const {
        return httpVersion;
    }

    bool isMethodImplemented() const {
        return isImplemented;
    }

private:
    StartLine(std::string method, std::string requestTarget, std::string httpVersion, bool isImplemented) :
            method(method), requestTarget(requestTarget), httpVersion(httpVersion),
            isImplemented(isImplemented) {};
    std::string method;
    bool isImplemented;
    std::string requestTarget;
    std::string httpVersion;
};


class HeaderField {
public:
    static std::optional<HeaderField> validateString(std::string s) {
        const std::regex headerFieldRegex(R"regex(([^:]+):([ ]*)([^\s]+)([ ]*))regex");
        std::cmatch m;
        if (!std::regex_match(s.c_str(), m, headerFieldRegex)) {
            return {};
        }
        /*
         * Field names other than explicitly described should be ignored. It's important to mark them
         * as such, because repeating non-ignored field names in one request is an error.
         * */
        static const std::array<std::string, 4> acceptedFieldnames = {"Connection", "Content-Length",
                                                                      "Server", "Content-Type"};
        auto it = std::find(acceptedFieldnames.begin(), acceptedFieldnames.end(), m[1]);
        bool ignored = (it == acceptedFieldnames.end());
        return HeaderField(m[1], m[3], ignored);
    }

    const std::string &getValue() const {
        return value;
    }

    const std::string &getName() const {
        return name;
    }

    const bool isIgnored() const {
        return ignored;
    }

    void setIsIgnored(bool value) {
        ignored = value;
    }

    bool operator==(const HeaderField &B) {
        return name == B.name && value == B.value && isIgnored() == B.isIgnored();
    }

private:
    HeaderField(std::string name, std::string value, bool ignored) : name(name), value(value), ignored(ignored) {}
    std::string name;
    std::string value;
    bool ignored;
};

class RequestHeaderField : public HeaderField {
public:
    static std::optional<HeaderField> validateString(std::string s) {
        auto res = HeaderField::validateString(s);
        if (!res) {
            return {};
        }
        static const std::array<std::string, 2> acceptedFieldnames = {"Connection", "Content-Length"};
        auto it = std::find(acceptedFieldnames.begin(), acceptedFieldnames.end(), res.value().getName());
        bool ignored = (it == acceptedFieldnames.end());
        /*
         * Additional check for Content-Length value -- it is mandatory that it's 0 in request field.
         * */
        if (res.value().getName() == "Content-Length" && res.value().getValue() != "0") {
            return {};
        }
        res.value().setIsIgnored(ignored);
        return res;
    }
};

class HttpMessage {
public:
    static std::string generateResponseStatusLine(std::string statusCode, std::string reasonPhrase) {
        return "HTTP/1.1 " +  statusCode + " " + reasonPhrase + "\n\r";
    }

    static std::optional<HttpMessage> validateHttpRequest(std::vector<std::string> s) {
        if (s.size() == 0) {
            return {};
        }
        auto startLine = StartLine::validateString(s[0]);
        if (!startLine.has_value()) {
            return {};
        }
        std::vector<HeaderField> headerFields;
        for (size_t i = 1; i < s.size(); i++) {
            auto headerField = RequestHeaderField::validateString(s[i]);
            if (!headerField.has_value()) {
                return {};
            }
            if (headerField.value().isIgnored()) {
                continue;
            }
            /*
             * Each not ignored header field's name has to be unique. If one appears more than once,
             * it's treated as "wrong argument" error.
             * */
            if (auto it = std::find(headerFields.begin(), headerFields.end(), headerField.value());
                it != headerFields.end()) {
                return {};
            }
            headerFields.push_back(headerField.value());
        }
        return HttpMessage(startLine.value(), headerFields);
    }

    const StartLine &getStartLine() const {
        return sl;
    }

    const std::vector<HeaderField> &getHeaderFields() const {
        return hf;
    }

private:
    StartLine sl;
    std::vector<HeaderField> hf;
    HttpMessage(const StartLine &sl, const std::vector<HeaderField> &hf) : sl(sl), hf(hf) {}
};


#endif //ZALICZENIOWE1_PARSE_REQUEST_H
