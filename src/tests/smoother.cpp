/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#include "SynthEngine.h"

#include <catch2/catch2.hpp>

#include <cmath>

namespace
{
// seconds for a unit step to reach 1 - 1/e
double timeConstant(float sampleRate)
{
    Smoother s;
    s.setSampleRate(sampleRate);
    s.setStep(1.f);

    const auto target = 1.f - std::exp(-1.f);
    int n{0};
    while (s.smoothStep() < target)
    {
        ++n;
        REQUIRE(n < static_cast<int>(sampleRate));
    }
    return n / static_cast<double>(sampleRate);
}
} // namespace

TEST_CASE("Smoother: the time constant is the same at every sample rate", "[smoother]")
{
    const auto at48k = timeConstant(48000.f);
    REQUIRE(at48k == Approx(0.00636).margin(0.00005));

    for (auto sr : {44100.f, 88200.f, 96000.f, 192000.f})
    {
        INFO("sample rate " << sr);
        REQUIRE(timeConstant(sr) == Approx(at48k).margin(1.0 / 44100.0));
    }
}
