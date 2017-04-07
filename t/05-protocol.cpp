#define CATCH_CONFIG_MAIN

#include <vector>

#include "catch.hpp"
#include "bredis/Protocol.hpp"

namespace r = bredis;

TEST_CASE("simple string", "[protocol]") {
    std::string ok = "+OK\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::string_holder_t>(r.result) == "OK");
};

TEST_CASE("empty string", "[protocol]") {
    std::string ok = "";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
};

TEST_CASE("non-finished ", "[protocol]") {
    std::string ok = "+OK";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
};

TEST_CASE("wrong start marker", "[protocol]") {
    std::string ok = "!OK";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
    auto err = boost::get<r::protocol_error_t>(r.result);
    REQUIRE(err == "wrong introduction");
};

TEST_CASE("simple number", "[protocol]") {
    std::string ok = ":55\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::int_result_t>(r.result) == 55);
};

TEST_CASE("large number", "[protocol]") {
    std::string ok = ":9223372036854775801\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::int_result_t>(r.result) == 9223372036854775801);
};

TEST_CASE("negative number", "[protocol]") {
    std::string ok = ":-922\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::int_result_t>(r.result) == -922);
};

TEST_CASE("wrong number", "[protocol]") {
    std::string ok = ":-9ab22\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
    auto err = boost::get<r::protocol_error_t>(r.result);
    REQUIRE(err == "bad lexical cast: source type value could not be "
                   "interpreted as target");
};

TEST_CASE("too large number", "[protocol]") {
    std::string ok = ":92233720368547758019223372036854775801\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
    auto err = boost::get<r::protocol_error_t>(r.result);
    REQUIRE(err == "bad lexical cast: source type value could not be "
                   "interpreted as target");
};

TEST_CASE("simple error", "[protocol]") {
    std::string ok = "-Ooops\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::error_holder_t>(r.result) == "Ooops");
};

TEST_CASE("nil", "[protocol]") {
    std::string ok = "$-1\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    r::nil_t nil;
    REQUIRE(boost::get<r::nil_t>(r.result) == nil);
};

TEST_CASE("malformed bulk string", "[protocol]") {
    std::string ok = "$-5\r\nsome\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
    std::string err = boost::get<r::protocol_error_t>(r.result).what;
    REQUIRE(err == "Value -5 in unacceptable for bulk strings");
};

TEST_CASE("some bulk string", "[protocol]") {
    std::string ok = "$4\r\nsome\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::string_holder_t>(r.result) == "some");
};

TEST_CASE("empty bulk string", "[protocol]") {
    std::string ok = "$0\r\n\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    REQUIRE(boost::get<r::string_holder_t>(r.result) == "");
};

TEST_CASE("patrial bulk string(1)", "[protocol]") {
    std::string ok = "$10\r\nsome\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
};

TEST_CASE("patrial bulk string(2)", "[protocol]") {
    std::string ok = "$4\r\nsome\r";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
};

TEST_CASE("empty array", "[protocol]") {
    std::string ok = "*0\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    auto &array = boost::get<r::array_result_t>(r.result);
    REQUIRE(array.elements.size() == 0);
};

TEST_CASE("null array", "[protocol]") {
    std::string ok = "*-1\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    r::nil_t nil;
    REQUIRE(boost::get<r::nil_t>(r.result) == nil);
};

TEST_CASE("malformed array", "[protocol]") {
    std::string ok = "*-4\r\nsome\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
    std::string err = boost::get<r::protocol_error_t>(r.result).what;
    REQUIRE(err == "Value -4 in unacceptable for arrays");
};

TEST_CASE("patrial rray", "[protocol]") {
    std::string ok = "*1\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(!r.consumed);
};

TEST_CASE("array: string, int, nil", "[protocol]") {
    std::string ok = "*3\r\n$4\r\nsome\r\n:5\r\n$-1\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    auto &array = boost::get<r::array_result_t>(r.result);
    REQUIRE(array.elements.size() == 3);

    REQUIRE(boost::get<r::string_holder_t>(array.elements[0]) == "some");
    REQUIRE(boost::get<r::int_result_t>(array.elements[1]) == 5);
    r::nil_t nil;
    REQUIRE(boost::get<r::nil_t>(array.elements[2]) == nil);
};

TEST_CASE("array of arrays: [int, int, int,], [str,err] ", "[protocol]") {
    std::string ok = "*2\r\n*3\r\n:1\r\n:2\r\n:3\r\n*2\r\n+Foo\r\n-Bar\r\n";
    r::parse_result_t r = r::Protocol::parse(ok);
    REQUIRE(r.consumed);
    auto &array = boost::get<r::array_result_t>(r.result);
    REQUIRE(array.elements.size() == 2);

    auto &a1 = boost::get<r::array_holder_t>(array.elements[0]);
    REQUIRE(a1.elements.size() == 3);
    REQUIRE(boost::get<r::int_result_t>(a1.elements[0]) == 1);
    REQUIRE(boost::get<r::int_result_t>(a1.elements[1]) == 2);
    REQUIRE(boost::get<r::int_result_t>(a1.elements[2]) == 3);

    auto &a2 = boost::get<r::array_holder_t>(array.elements[1]);
    REQUIRE(a2.elements.size() == 2);
    REQUIRE(boost::get<r::string_holder_t>(a2.elements[0]) == "Foo");
    REQUIRE(boost::get<r::error_holder_t>(a2.elements[1]) == "Bar");
};

TEST_CASE("serialize", "[protocol]") {
    std::stringstream buff;
    std::string queue = "fmm.cheap-travles2";
    std::vector<boost::string_ref> args({boost::string_ref(queue)});
    r::Protocol::serialize(buff, "LLEN", args);
    std::string expected("*2\r\n$4\r\nLLEN\r\n$18\r\nfmm.cheap-travles2\r\n");
    REQUIRE(buff.str() == expected);
};
