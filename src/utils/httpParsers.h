#ifndef ZALICZENIOWE1_PARSE_REQUEST_H
#define ZALICZENIOWE1_PARSE_REQUEST_H

#include <array>
#include <regex>
#include <optional>
#include <iostream>
#include <vector>

class StartLine {
public:
    static std::optional<StartLine> validateString(const std::string &s) {
        const std::regex startlineRegex(R"regex((\w+) (/[a-zA-Z0-9.-/]+) (HTTP/1[.]1))regex");
        std::cmatch m;
        if (!std::regex_match(s.c_str(), m, startlineRegex)) {
            return {};
        }
        return StartLine(m[1], m[2], m[3]);
    }

    const std::string &getMethod() const {
        return method;
    }

    const std::string &getRequestTarget() const {
        return requestTarget;
    }

    const std::string &getHttpVersion() const {
        return httpVersion;
    }

private:
    StartLine(const std::string &method, const std::string &requestTarget,
              const std::string &httpVersion) :
            method(method), requestTarget(requestTarget), httpVersion(httpVersion) {};
    std::string method;
    std::string requestTarget;
    std::string httpVersion;
};


class HeaderField {
public:
    static std::optional<HeaderField> validateString(const std::string &s) {
        const std::regex headerFieldRegex(R"regex(([^\s:]+):( *)(.+?)( *))regex");
        std::cmatch m;
        if (!std::regex_match(s.c_str(), m, headerFieldRegex)) {
            return {};
        }
        /*
         * Field names other than explicitly described should be ignored. It's important to mark them
         * as such, because repeating non-ignored field names in one request is an error.
         * */
        std::string method = m[1];
        std::string value = m[3];
        std::transform(method.begin(), method.end(), method.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        static const std::array<std::string, 4> acceptedFieldnames = {"connection", "content-length",
                                                                      "server", "content-Type"};
        auto it = std::find(acceptedFieldnames.begin(), acceptedFieldnames.end(), method);
        bool ignored = (it == acceptedFieldnames.end());
        return HeaderField(method, value, ignored);
    }

    static std::optional<HeaderField> validateRequestString(const std::string &s) {
        auto res = validateString(s);
        if (!res) {
            return {};
        }

        static const std::array<std::string, 2> acceptedFieldnames = {"connection", "content-length"};
        auto it = std::find(acceptedFieldnames.begin(), acceptedFieldnames.end(), res.value().getName());
        bool ignored = (it == acceptedFieldnames.end());
        /*
         * Additional check for Content-Length value -- it is mandatory that it's 0 in request field.
         * */
        if (res.value().getName() == "content-length" && res.value().getValue() != "0") {
            return {};
        }
        res.value().setIsIgnored(ignored);
        return res;
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

    void setIsIgnored(bool ign) {
        ignored = ign;
    }

    bool operator==(const HeaderField &hf) {
        return name == hf.name;
    }

private:
    HeaderField(std::string name, std::string value, bool ignored) : name(name), value(value), ignored(ignored) {}
    std::string name;
    std::string value;
    bool ignored;
};

class HttpMessage {
public:
    static std::string generateResponseStatusLine(const std::string &statusCode, const std::string &reasonPhrase) {
        return "HTTP/1.1 " +  statusCode + " " + reasonPhrase;
    }

    static std::optional<HttpMessage> validateHttpRequest(const std::vector<std::string> &s) {
        if (s.size() == 0) {
            return {};
        }
        auto startLine = StartLine::validateString(s[0]);
        if (!startLine.has_value()) {
            return {};
        }
        std::vector<HeaderField> headerFields;
        for (size_t i = 1; i < s.size(); i++) {
            auto headerField = HeaderField::validateRequestString(s[i]);
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
            if (std::find(headerFields.begin(), headerFields.end(), headerField.value()) != headerFields.end()) {
                return {};
            }
            headerFields.push_back(headerField.value());
        }
        return HttpMessage(startLine.value(), headerFields);
    }

    static std::string generateHttpString(const std::vector<std::string> tokens) {
        std::string result = "";
        for (const auto &token : tokens) {
            result += (token + "\r\n");
        }
        result += "\r\n";
        return result;
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
