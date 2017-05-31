#include <UnitTests\Catch.h>

#include <string>

#include <Utils\StringUtils.h>

TEST_CASE("StringUtils")
{
    SECTION("AnsiToWideString")
    {
        std::string sourceString("source string");
        std::wstring destinationWString(L"source string");
        std::wstring outputString;

        BRE::StringUtils::AnsiToWideString(sourceString, 
                                           outputString);

        REQUIRE(destinationWString == outputString);
    }

    SECTION("AnsiToWideString version 2")
    {
        std::string sourceString("source string");
        std::wstring destinationWString(L"source string");
        std::wstring outputString;

        outputString = BRE::StringUtils::AnsiToWideString(sourceString);

        REQUIRE(destinationWString == outputString);
    }
}
