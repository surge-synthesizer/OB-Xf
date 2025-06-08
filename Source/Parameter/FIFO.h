/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_PARAMETER_FIFO_H
#define OBXF_SRC_PARAMETER_FIFO_H
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cstring>

#ifdef _MSC_VER
#define OBXF_SRC_PARAMETER_FIFO_H
#include <cstring>
#else
#include <cstring>
#endif

constexpr size_t kMaxParamIdLen = 32;

struct ParameterChange
{
    char parameterID[kMaxParamIdLen]{};
    float newValue;

    ParameterChange() : parameterID{0}, newValue{0.f} {}
    ParameterChange(const juce::String &id, const float value) : newValue(value)
    {
#ifdef USE_STRNCPY_S
        strncpy_s(parameterID, kMaxParamIdLen, id.toRawUTF8(), kMaxParamIdLen);
#else
        std::strncpy(parameterID, id.toRawUTF8(), kMaxParamIdLen);
#endif
        parameterID[kMaxParamIdLen - 1] = '\0';
    }
};

template <size_t Capacity> class FIFO
{
  public:
    FIFO() : abstractFIFO{Capacity} {}

    void clear() { abstractFIFO.reset(); }

    size_t getFreeSpace() const { return abstractFIFO.getFreeSpace(); }

    bool pushParameter(const juce::String &parameterID, float newValue)
    {
        if (abstractFIFO.getFreeSpace() == 0)
            return false;
        auto scope = abstractFIFO.write(1);
        if (scope.blockSize1 > 0)
            buffer[scope.startIndex1] = ParameterChange(parameterID, newValue);
        if (scope.blockSize2 > 0)
            buffer[scope.startIndex2] = ParameterChange(parameterID, newValue);
        return true;
    }

    std::pair<bool, ParameterChange> popParameter()
    {
        if (abstractFIFO.getNumReady() == 0)
            return {};
        auto scope = abstractFIFO.read(1);
        if (scope.blockSize1 > 0)
            return {true, buffer[scope.startIndex1]};
        if (scope.blockSize2 > 0)
            return {true, buffer[scope.startIndex2]};
        return {false, ParameterChange()};
    }

  private:
    juce::AbstractFifo abstractFIFO;
    std::array<ParameterChange, Capacity> buffer;

    JUCE_DECLARE_NON_COPYABLE(FIFO)
    JUCE_DECLARE_NON_MOVEABLE(FIFO)
    JUCE_LEAK_DETECTOR(FIFO)
};
#endif // OBXF_SRC_PARAMETER_FIFO_H
