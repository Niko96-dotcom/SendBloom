#include <ui/PedalFaceplatePaint.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("footswitch pressed overlay follows press/amount not connection",
           "[send][SEND-06]")
{
    using sendbloom::ui::shouldDrawFootswitchPressedOverlay;

    REQUIRE_FALSE (shouldDrawFootswitchPressedOverlay (false, 0.0f, 0.0f));
    REQUIRE (shouldDrawFootswitchPressedOverlay (true, 0.0f, 0.0f));
    REQUIRE (shouldDrawFootswitchPressedOverlay (false, 0.5f, 0.0f));
    REQUIRE (shouldDrawFootswitchPressedOverlay (false, 0.0f, 0.5f));
    REQUIRE_FALSE (shouldDrawFootswitchPressedOverlay (false, 0.0005f, 0.0005f));
}
