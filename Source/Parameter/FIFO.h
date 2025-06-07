// FIFO.h
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cstring>

#ifdef _MSC_VER
    #define USE_STRNCPY_S
    #include <cstring>
#else
    #include <cstring>
#endif

constexpr size_t kMaxParamIdLen = 32;

struct ParameterChange {
    char parameterID[kMaxParamIdLen]{};
    float newValue;

    ParameterChange() : parameterID{0}, newValue{0.f} {}
    ParameterChange(const juce::String& id, const float value) : newValue(value) {
#ifdef USE_STRNCPY_S
        strncpy_s(parameterID, kMaxParamIdLen, id.toRawUTF8(), kMaxParamIdLen);
#else
        std::strncpy(parameterID, id.toRawUTF8(), kMaxParamIdLen);
#endif
        parameterID[kMaxParamIdLen - 1] = '\0';
    }
};

template<size_t Capacity>
class FIFO {
public:
    FIFO() : abstractFIFO{Capacity} {}

    void clear() { abstractFIFO.reset(); }

    size_t getFreeSpace() const { return abstractFIFO.getFreeSpace(); }

    bool pushParameter(const juce::String& parameterID, float newValue) {
        if (abstractFIFO.getFreeSpace() == 0)
            return false;
        auto scope = abstractFIFO.write(1);
        if (scope.blockSize1 > 0)
            buffer[scope.startIndex1] = ParameterChange(parameterID, newValue);
        if (scope.blockSize2 > 0)
            buffer[scope.startIndex2] = ParameterChange(parameterID, newValue);
        return true;
    }

    std::pair<bool, ParameterChange> popParameter() {
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