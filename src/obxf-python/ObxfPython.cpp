#include "ObxfPython.h"

#ifdef OBXF_HEADLESS
juce::AudioProcessorEditor *ObxfAudioProcessor::createEditor() { return nullptr; }
bool ObxfAudioProcessor::hasEditor() const { return false; }
#endif

// Prefix _ if using shared object within a Python package built with scikit-build
#ifdef SKBUILD
PYBIND11_MODULE(_obxfpy, m)
#else
PYBIND11_MODULE(obxfpy, m)
#endif
{
    m.doc() = "Python bindings for OB-Xf";
    obxf::python::registerObxfPython(m);
}