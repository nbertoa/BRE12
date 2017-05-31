#include <UnitTests\Catch.h>

#include <MathUtils\MathUtils.h>

TEST_CASE("MathUtils")
{
    SECTION("RandomFloatInInverval")
    {
        const float bottomValue = -10.0f;
        const float topValue = 10.0f;

        for (std::uint32_t i = 0U; i < 1000U; ++i) {
            const float randomValue = BRE::MathUtils::RandomFloatInInterval(bottomValue,
                                                                            topValue);
                      
            bool b = bottomValue < randomValue || BRE::MathUtils::AreEqual(bottomValue, randomValue);
            REQUIRE(b);
            b = randomValue < topValue || BRE::MathUtils::AreEqual(topValue, randomValue);
            REQUIRE(b);
        }
    }

    SECTION("RandomIntegerInInterval")
    {
        const std::int32_t bottomValue = -10;
        const std::int32_t topValue = 10;

        for (std::uint32_t i = 0U; i < 1000U; ++i) {
            const std::int32_t randomValue = BRE::MathUtils::RandomIntegerInInterval(bottomValue,
                                                                                     topValue);

            REQUIRE(bottomValue <= randomValue);
            REQUIRE(randomValue <= topValue);
        }
    }

    SECTION("Min")
    {
        const float f1 = 1.0f;
        const float f2 = 5.0f;
        const float minFloat = f1;

        const float result = BRE::MathUtils::Min(f1, f2);
        REQUIRE(BRE::MathUtils::AreEqual(minFloat, result));
    }

    SECTION("Max")
    {
        const float f1 = 1.0f;
        const float f2 = 5.0f;
        const float maxFloat = f2;

        const float result = BRE::MathUtils::Max(f1, f2);
        REQUIRE(BRE::MathUtils::AreEqual(maxFloat, result));
    }

    SECTION("Lerp")
    {
        const float f1 = -5.0f;
        const float f2 = 5.0f;
        const float middle = (f2 + f1) * 0.5f;

        float res = BRE::MathUtils::Lerp(f1, f2, 0.0f);
        REQUIRE(BRE::MathUtils::AreEqual(f1, res));

        res = BRE::MathUtils::Lerp(f1, f2, 1.0f);
        REQUIRE(BRE::MathUtils::AreEqual(f2, res));

        res = BRE::MathUtils::Lerp(f1, f2, 0.5f);
        REQUIRE(BRE::MathUtils::AreEqual(middle, res));

        res = BRE::MathUtils::Lerp(f2, f1, 0.0f);
        REQUIRE(BRE::MathUtils::AreEqual(f2, res));

        res = BRE::MathUtils::Lerp(f2, f1, 1.0f);
        REQUIRE(BRE::MathUtils::AreEqual(f1, res));

        res = BRE::MathUtils::Lerp(f2, f1, 0.5f);
        REQUIRE(BRE::MathUtils::AreEqual(middle, res));
    }

    SECTION("Clamp")
    {
        const float f1 = -5.0f;
        const float f2 = 5.0f;

        float res = BRE::MathUtils::Clamp(-6.0f, f1, f2);
        REQUIRE(BRE::MathUtils::AreEqual(f1, res));

        res = BRE::MathUtils::Clamp(6.0f, f1, f2);
        REQUIRE(BRE::MathUtils::AreEqual(f2, res));

        res = BRE::MathUtils::Clamp(0.0f, f1, f2);
        REQUIRE(BRE::MathUtils::AreEqual(0.0f, res));
    }
}