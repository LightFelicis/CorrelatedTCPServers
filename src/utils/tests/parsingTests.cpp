#include "../httpParsers.h"

#include <gtest/gtest.h>

TEST(Startline_parsing, simple) {
    std::optional<StartLine> res = StartLine::validateString("GET /pliK1/2 HTTP/1.1");
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().getMethod(), "GET");
    ASSERT_EQ(res.value().getHttpVersion(), "HTTP/1.1");
    ASSERT_EQ(res.value().getRequestTarget(), "/pliK1/2");
}

TEST(Startline_parsing, invalid_request_http_version) {
    std::optional<StartLine> res = StartLine::validateString("HEAD /plik HTTP/1,1");
    ASSERT_FALSE(res);
}

TEST(Startline_parsing, too_many_whitespaces) {
    std::optional<StartLine> res = StartLine::validateString("HEAD  /plik HTTP/1.1");
    ASSERT_FALSE(res);
}

TEST(Startline_parsing, no_backslash_in_path) {
    std::optional<StartLine> res = StartLine::validateString("HEAD plik HTTP/1.1");
    ASSERT_FALSE(res);
}

TEST(HeaderField_parsing, simple) {
    std::optional<HeaderField> res = HeaderField::validateString("Content-Length:  0  ");
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().getName(), "content-length");
    ASSERT_EQ(res.value().getValue(), "0");
    ASSERT_FALSE(res.value().isIgnored());
    std::optional<HeaderField> res2 = HeaderField::validateString("connection:close");
    ASSERT_TRUE(res2);
    ASSERT_EQ(res2.value().getName(), "connection");
    ASSERT_EQ(res2.value().getValue(), "close");
    ASSERT_FALSE(res2.value().isIgnored());
}

TEST(HeaderField_parsing, missing_colon) {
    std::optional<HeaderField> res = HeaderField::validateString("ala makota");
    ASSERT_FALSE(res);
}

TEST(HeaderField_parsing, whitespace_error) {
    std::optional<HeaderField> res = HeaderField::validateString("ala : makota");
    ASSERT_FALSE(res);
}

TEST(RequestHeaderField_parsing, invalid_content_length) {
    std::optional<HeaderField> res = HeaderField::validateRequestString("content-length:  12");
    ASSERT_FALSE(res);
}

TEST(HttpMessage_parsing, simple) {
    std::optional<HttpMessage> res = HttpMessage::validateHttpRequest({"GET /plik HTTP/1.1",
                                                                       "Content-Length: 0",
                                                                       "Server: spaaaam",
                                                                       "Server: spaaaam2",
                                                                       "Connection: close"});
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().getHeaderFields().size(), 2);
    ASSERT_TRUE(res.value().getStartLine().getMethod() == "GET");
}

TEST(HttpMessage_parsing, duplicated) {
    std::optional<HttpMessage> res = HttpMessage::validateHttpRequest({"GET /plik HTTP/1.1",
                                                                       "Connection: 12",
                                                                       "Connection: close"});
    ASSERT_FALSE(res);
}
