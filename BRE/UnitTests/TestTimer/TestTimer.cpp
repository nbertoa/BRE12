#include <UnitTests\Catch.h>

#include <MathUtils\MathUtils.h>
#include <Timer\Timer.h>

TEST_CASE("Timer")
{
    BRE::Timer timer;

    SECTION("At construction, the delta time in seconds must be zero")
    {
        REQUIRE(BRE::MathUtils::AreEqual(0.0f, 
                                         timer.GetDeltaTimeInSeconds()));
    }

    SECTION("If we call Tick() method, then GetDeltaTimeInSeconds must not be zero")
    {
        timer.Tick();
        REQUIRE(timer.GetDeltaTimeInSeconds() > 0.0f);
    }

    SECTION("If we call Reset(), then GetDeltaTimeInSeconds must be zero again")
    {
        timer.Reset();
        REQUIRE(BRE::MathUtils::AreEqual(0.0f, 
                                         timer.GetDeltaTimeInSeconds()));
    }
}
