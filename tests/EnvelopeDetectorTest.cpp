#include <EnvelopeDetector.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("EnvelopeDetector rises on burst", "[gate][EnvelopeDetector]")
{
    sendbloom::EnvelopeDetector env;
    env.prepare (48000.0, 5.0f, 10.0f);

    for (int i = 0; i < 4800; ++i)
        env.process (0.5f);

    REQUIRE (env.getEnvelope() > 0.4f);
}

TEST_CASE ("EnvelopeDetector decays in silence", "[gate][EnvelopeDetector]")
{
    sendbloom::EnvelopeDetector env;
    env.prepare (48000.0, 5.0f, 10.0f);

    for (int i = 0; i < 4800; ++i)
        env.process (0.5f);

    for (int i = 0; i < 4800; ++i)
        env.process (0.0f);

    REQUIRE (env.getEnvelope() < 0.01f);
}
