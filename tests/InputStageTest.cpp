#include <InputStage.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("InputStage unity gain passes sample", "[io][InputStage]")
{
    sendbloom::InputStage stage;
    stage.prepare (48000.0);

    const auto out = stage.processSample (0.25f, 1.0f);
    REQUIRE (out == Catch::Approx (0.25f));
}

TEST_CASE ("InputStage high gain triggers clip hold", "[io][InputStage]")
{
    sendbloom::InputStage stage;
    stage.prepare (48000.0);

    for (int i = 0; i < 100; ++i)
        stage.processSample (1.0f, 8.0f);

    REQUIRE (stage.isClipHoldActive());
}

TEST_CASE ("InputStage clip hold persists then clears", "[io][InputStage]")
{
    sendbloom::InputStage stage;
    stage.prepare (48000.0);

    for (int i = 0; i < 100; ++i)
        stage.processSample (1.0f, 8.0f);

    REQUIRE (stage.isClipHoldActive());

    for (int i = 0; i < 2400; ++i)
        stage.processSample (0.0f, 1.0f);

    REQUIRE_FALSE (stage.isClipHoldActive());
}

TEST_CASE ("InputStage soft clip limits peak", "[io][InputStage]")
{
    sendbloom::InputStage stage;
    stage.prepare (48000.0);

    const auto out = stage.processSample (1.0f, 10.0f);
    REQUIRE (std::abs (out) <= 0.708f);
}
