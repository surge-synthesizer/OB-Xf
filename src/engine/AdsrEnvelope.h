/*
 * OB-Xf - a continuation of the last open source version of OB-Xd.
 *
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more info,
 * see "HISTORY.md" in the root of this repository.
 *
 * This repository is a successor to the last open source release,
 * a version marked as 2.11. Copyright 2013-2025 by the authors
 * as indicated in the original release, and subsequent authors
 * per the GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_ADSRENVELOPE_H
#define OBXF_SRC_ENGINE_ADSRENVELOPE_H

class ADSREnvelope
{
  public:
    // See https://github.com/surge-synthesizer/OB-Xf/issues/116#issuecomment-2981640815
    // for a discussion of these. Basically atkCoefStart is fixed, atkCoefEnd is an overshoot
    // speed factor, with 1 being no overshoot speed, and atkValueEnd is the distance from
    // 1.0 before an attack ends. The difference between atkCoefStart and atkValueEnd
    // creates a timing mismatch. Also see scripts/attacksim.py for how I noodled with
    // this
    static constexpr float atkCoefStart{0.001f}, atkCoefEnd{1.3f}, atkValueEnd{0.1f};
    static constexpr float atkTimeAdjustment{1.f / 3.f};
    static constexpr float msToSec{0.001f};
    static constexpr float defaultTime{0.0001f}, defaultLevel{1.f};

  private:
    enum State
    {
        Attack = 1,
        Decay = 2,
        Sustain = 3,
        Release = 4,
        Silent = 5
    } state{Silent};

    // clang-format off
    struct Parameters
    {
        float a{defaultTime};
        float d{defaultTime};
        float s{defaultLevel};
        float r{defaultTime};
    } orig,   // parameter values before adding the slop offset
      offset, // well this should be self explanatory
      par;    // final parameter values (orig + offset)
    // clang-format on

    float coef{0.f};
    float output{0.f};
    float sampleRate{1.f};
    float offsetFactor{1.f};

  public:
    ADSREnvelope() {}

    void ResetEnvelopeState()
    {
        output = 0.0f;
        state = State::Silent;
    }

    void setSampleRate(float sr) { sampleRate = sr; }

    void setEnvOffsets(float v)
    {
        offsetFactor = v;

        setAttack(orig.a);
        setDecay(orig.d);
        setSustain(orig.s);
        setRelease(orig.r);
    }

    void setAttackCurve(float c) {}

    void setAttack(float a)
    {
        orig.a = a;
        offset.a = a / atkTimeAdjustment;
        par.a = a * offsetFactor / atkTimeAdjustment;

        if (state == State::Attack)
        {
            coef = static_cast<float>((log(atkCoefStart) - log(atkCoefEnd)) /
                                      (sampleRate * par.a * msToSec));
        }
    }

    void setDecay(float d)
    {
        orig.d = d;
        offset.d = d;
        par.d = d * offsetFactor;

        if (state == State::Decay)
        {
            coef = static_cast<float>((log(juce::jmin(par.s + 0.0001, 0.99)) - log(1.0)) /
                                      (sampleRate * par.d * msToSec));
        }
    }

    void setSustain(float s)
    {
        orig.s = s;
        offset.s = s;
        par.s = s;

        if (state == State::Decay)
        {
            coef = static_cast<float>((log(juce::jmin(par.s + 0.0001, 0.99)) - log(1.0)) /
                                      (sampleRate * par.d * msToSec));
        }
    }

    void setRelease(float r)
    {
        orig.r = r;
        offset.r = r;
        par.r = r * offsetFactor;

        if (state == State::Release)
        {
            coef = static_cast<float>((log(0.00001) - log(output + 0.0001)) /
                                      (sampleRate * par.r * msToSec));
        }
    }

    void triggerAttack()
    {
        state = State::Attack;
        coef = static_cast<float>((log(atkCoefStart) - log(atkCoefEnd)) /
                                  (sampleRate * par.a * msToSec));
    }

    void triggerRelease()
    {
        if (state != State::Release)
        {
            coef = static_cast<float>((log(0.00001) - log(output + 0.0001)) /
                                      (sampleRate * par.r * msToSec));
        }

        state = State::Release;
    }

    inline bool isActive() { return state != State::Silent; }

    inline bool isGated() { return state != State::Silent && state != State::Release; }

    inline float processSample()
    {
        switch (state)
        {
        case State::Attack:
            if (output - 1.f > -atkValueEnd)
            {
                output = juce::jmin(output, 0.99f);
                state = State::Decay;
                coef = static_cast<float>((log(juce::jmin(par.s + 0.0001, 0.99)) - log(1.0)) /
                                          (sampleRate * par.d * msToSec));
                goto dec;
            }
            else
            {
                output = output - (1.f - output) * coef;
            }
            break;
        case State::Decay:
        dec:
            if (output - par.s < 10e-6f)
            {
                state = State::Sustain;
            }
            else
            {
                output = output + output * coef;
            }
            break;
        case State::Sustain:
            output = juce::jmin(par.s, 0.9f);
            break;
        case State::Release:
            if (output > 20e-6f)
                output = output + (output * coef) + dc;
            else
                state = State::Silent;
            break;
        case State::Silent:
            output = 0.f;
            break;
        }

        return output;
    }
};

#endif // OBXF_SRC_ENGINE_ADSRENVELOPE_H
