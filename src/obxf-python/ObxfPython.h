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

#ifndef OBXF_SRC_OBXFPYTHON_OBXFPYTHON_H
#define OBXF_SRC_OBXFPYTHON_OBXFPYTHON_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <stdexcept>

#include <ObxfProcessor.h>
#include <parameter/ParameterCoordinator.h>

namespace py = pybind11;

namespace obxf::python
{

class ObxfPyEngine
{
  public:
    ObxfPyEngine(double sampleRate) : processor() { processor.prepareToPlay(sampleRate, 512); }

    // --- MIDI ----------------------------------------------------------------

    void noteOn(int note, int velocity, int8_t channel = 0)
    {
        processor.getSynth().processNoteOn(note, static_cast<float>(velocity) / 127.f, channel);
    }

    void noteOff(int note, int velocity, int8_t channel = 0)
    {
        processor.getSynth().processNoteOff(note, static_cast<float>(velocity) / 127.f, channel);
    }

    void pitchBend(float value) { processor.getSynth().processPitchWheel(value); }
    void modWheel(float value) { processor.getSynth().processModWheel(value); }
    void sustainOn() { processor.getSynth().sustainOn(); }
    void sustainOff() { processor.getSynth().sustainOff(); }
    void allNotesOff() { processor.getSynth().allNotesOff(); }
    void allSoundOff() { processor.getSynth().allSoundOff(); }

    // --- Parameters ----------------------------------------------------------

    void setParam(const std::string &paramId, float value)
    {
        const auto &handlers = obxf::getHandlerMap();
        auto it = handlers.find(paramId);

        if (it != handlers.end())
        {
            it->second(processor.getSynth(), value);
            processor.getActiveProgram().values[juce::String(paramId)] = value;
        }
    }

    float getParam(const std::string &paramId) const
    {
        const auto &values = processor.getActiveProgram().values;
        auto it = values.find(juce::String(paramId));

        if (it != values.end())
        {
            return it->second;
        }

        return 0.f;
    }

    // Return every parameter ID the handler map knows about
    // Useful for introspection and building Python-side wrappers.
    py::list getParamIds() const
    {
        py::list ids;
        for (const auto &[id, _] : obxf::getHandlerMap())
            ids.append(id);
        return ids;
    }

    // --- MPE -----------------------------------------------------------------

    void setMpeEnabled(bool enabled) { processor.setMpeEnabled(enabled); }

    void setMpePitchBendRange(int range) { processor.setMpePitchBendRange(range); }

    bool getMpeEnabled() { return processor.getMidiHandler().mpeEnabled.load(); }

    int getMpePitchBendRange() { return processor.getMidiHandler().mpePitchBendRange.load(); }

    static bool isValidSourceTargetCombo(MatrixSource source, const std::string &target)
    {
        // Envelope attack targets: Strike only
        if (target == ID::AmpEnvAttack || target == ID::FilterEnvAttack)
        {
            return source == MatrixSource::Strike;
        }

        // Envelope release targets: Lift only
        if (target == ID::AmpEnvRelease || target == ID::FilterEnvRelease)
        {
            return source == MatrixSource::Lift;
        }

        return isValidMatrixTarget(target);
    }

    // Map dimension + local index (0 or 1) to the flat row index
    static int matrixRowIndex(MatrixSource source, int localIndex)
    {
        if (localIndex < 0 || localIndex > 1)
            throw std::out_of_range("MPE mod matrix row index must be 0 or 1!");

        switch (source)
        {
        case MatrixSource::Strike:
            return 0 + localIndex;
        case MatrixSource::Lift:
            return 2 + localIndex;
        case MatrixSource::Press:
            return 4 + localIndex;
        case MatrixSource::Slide:
            return 6 + localIndex;
        default:
            throw std::invalid_argument(
                "MPE mod matrix onnly supports Strike, Lift, Press and Slide as sources!");
        }
    }

    void setMatrixRow(const std::string &dimension, int localIndex, const std::string &target,
                      float depth)
    {
        auto src = matrixSourceFromString(dimension);

        if (!isValidSourceTargetCombo(src, target))
            throw std::invalid_argument(target + "' is not a valid destination for MPE dimension " +
                                        dimension + "!");

        int idx = matrixRowIndex(src, localIndex);

        MatrixRow row;
        row.source = src;
        row.target = target;
        row.depth = depth;

        processor.getSynth().getMotherboard()->voiceMatrix.rows[idx] = row;
    }

    void clearMatrixRow(const std::string &dimension, int localIndex)
    {
        int idx = matrixRowIndex(matrixSourceFromString(dimension), localIndex);
        processor.getSynth().getMotherboard()->voiceMatrix.rows[idx] = MatrixRow{};
    }

    void clearMatrixDimension(const std::string &dimension)
    {
        auto src = matrixSourceFromString(dimension);
        int base = matrixRowIndex(src, 0);
        processor.getSynth().getMotherboard()->voiceMatrix.rows[base] = MatrixRow{};
        processor.getSynth().getMotherboard()->voiceMatrix.rows[base + 1] = MatrixRow{};
    }

    py::list getMatrix() const
    {
        py::list result;
        const auto &rows = processor.getSynth().getMotherboard()->voiceMatrix.rows;

        // Only iterate the 8 managed rows
        const std::pair<MatrixSource, int> layout[] = {
            {MatrixSource::Strike, 0}, {MatrixSource::Strike, 1}, {MatrixSource::Lift, 0},
            {MatrixSource::Lift, 1},   {MatrixSource::Press, 0},  {MatrixSource::Press, 1},
            {MatrixSource::Slide, 0},  {MatrixSource::Slide, 1},
        };

        for (auto &[src, localIdx] : layout)
        {
            int flatIdx = matrixRowIndex(src, localIdx);
            const auto &row = rows[flatIdx];

            if (!row.isActive())
            {
                continue;
            }

            py::dict d;
            d["dimension"] = matrixSourceToString(src);
            d["row"] = localIdx;
            d["target"] = row.target;
            d["depth"] = row.depth;

            result.append(d);
        }
        return result;
    }

    // --- Paths ---------------------------------------------------------------

    // Factory patches folder (resolves system vs local install automatically)
    std::string getFactoryPatchesPath() const
    {
        return processor.getUtils()
            .getFactoryFolderInUse()
            .getChildFile("Patches")
            .getFullPathName()
            .toStdString();
    }

    // User patches folder (documents/Surge Synth Team/OB-Xf/Patches)
    std::string getUserPatchesPath() const
    {
        return processor.getUtils()
            .getDocumentFolder()
            .getChildFile("Patches")
            .getFullPathName()
            .toStdString();
    }

    // --- Patch I/O -----------------------------------------------------------

    void loadPatch(const std::string &path)
    {
        juce::File f(path);

        if (!f.existsAsFile())
        {
            throw std::runtime_error("Patch file not found: " + path);
        }

        juce::MemoryBlock mb;

        if (!f.loadFileAsData(mb))
        {
            throw std::runtime_error("Could not read patch file: " + path);
        }

        processor.getStateManager().loadFromMemoryBlock(mb);

        const auto &handlers = obxf::getHandlerMap();

        for (const auto &[paramId, value] : processor.getActiveProgram().values)
        {
            auto it = handlers.find(paramId.toStdString());

            if (it != handlers.end())
            {
                it->second(processor.getSynth(), value.load());
            }
        }
    }

    void savePatch(const std::string &path)
    {
        juce::MemoryBlock mb;
        processor.getStateInformation(mb);

        juce::File f(path);

        if (!f.replaceWithData(mb.getData(), mb.getSize()))
        {
            throw std::runtime_error("Could not write patch file: " + path);
        }
    }

    std::string findPatch(const std::string &name) const
    {
        const std::vector<juce::File> searchRoots = {juce::File(getFactoryPatchesPath()),
                                                     juce::File(getUserPatchesPath())};

        std::string filename = name;

        if (!juce::String(filename).endsWithIgnoreCase(".fxp"))
        {
            filename += ".fxp";
        }

        for (const auto &root : searchRoots)
        {
            auto result = root.findChildFiles(juce::File::findFiles, true, filename);

            if (!result.isEmpty())
            {
                return result[0].getFullPathName().toStdString();
            }
        }

        throw std::runtime_error("Patch not found: " + name);
    }

    // --- Audio ---------------------------------------------------------------

    py::tuple process(int nSamples)
    {
        auto outL = py::array_t<float>(nSamples);
        auto outR = py::array_t<float>(nSamples);

        auto l = outL.mutable_unchecked<1>();
        auto r = outR.mutable_unchecked<1>();

        {
            py::gil_scoped_release release;
            SynthEngine &synth = processor.getSynth();
            for (int i = 0; i < nSamples; ++i)
            {
                float ls = 0.f, rs = 0.f;
                synth.processSample(&ls, &rs);
                l(i) = ls;
                r(i) = rs;
            }
        }

        return py::make_tuple(outL, outR);
    }

  private:
    ObxfAudioProcessor processor;
};

// -------------------------------------------------------------------------

inline void registerObxfPython(py::module_ &m)
{
    py::class_<ObxfPyEngine>(m, "ObxfEngine", "Create an OB-Xf instance.")

        .def(py::init<double>(), py::arg("sample_rate") = 44100.0,
             "Create an instance of OB-Xf engine running at specified sample rate in Hz")

        // MIDI
        .def("note_on", &ObxfPyEngine::noteOn, py::arg("note"), py::arg("velocity"),
             py::arg("channel") = 0, "Sends a note on message, velocity 0..127.")
        .def("note_off", &ObxfPyEngine::noteOff, py::arg("note"), py::arg("velocity"),
             py::arg("channel") = 0, "Sends a note off message, velocity 0..127.")
        .def("pitch_bend", &ObxfPyEngine::pitchBend, py::arg("value"))
        .def("mod_wheel", &ObxfPyEngine::modWheel, py::arg("value"))
        .def("sustain_on", &ObxfPyEngine::sustainOn)
        .def("sustain_off", &ObxfPyEngine::sustainOff)
        .def("all_notes_off", &ObxfPyEngine::allNotesOff)
        .def("all_sound_off", &ObxfPyEngine::allSoundOff)

        // Parameters
        .def("set_param", &ObxfPyEngine::setParam, py::arg("param_id"), py::arg("value"),
             "Set a parameter by its ID string.")
        .def("get_param", &ObxfPyEngine::getParam, py::arg("param_id"),
             "Get the last set value for a parameter ID.")
        .def("get_param_ids", &ObxfPyEngine::getParamIds,
             "Return a list of all known parameter ID strings.")

        // MPE
        .def("set_mpe_modulation", &ObxfPyEngine::setMatrixRow, py::arg("dimension"),
             py::arg("row"), py::arg("target"), py::arg("depth"),
             "Set a MPE mod matrix routing for the specified MPE dimension.\n"
             "dimension: Strike, Lift, Press, Slide\n"
             "row:       0 or 1\n"
             "target:    parameter ID string\n"
             "depth:     -1..1")
        .def("unset_mpe_modulation", &ObxfPyEngine::clearMatrixRow, py::arg("dimension"),
             py::arg("row"))
        .def("reset_mpe_dimension", &ObxfPyEngine::clearMatrixDimension, py::arg("dimension"),
             "Reset both MPE mod matrix rows for the specified MPE dimension.")
        .def("get_matrix", &ObxfPyEngine::getMatrix,
             "Return all active MPE mod matrix assignments as a list of dicts with "
             "dimension/row/target/depth.")

        // Paths
        .def("get_factory_patches_path", &ObxfPyEngine::getFactoryPatchesPath,
             "Return the factory patches folder path.")
        .def("get_user_patches_path", &ObxfPyEngine::getUserPatchesPath,
             "Return the user patches folder path.")

        // Patches
        .def("load_patch", &ObxfPyEngine::loadPatch, py::arg("path"), "Load an FXP patch.")
        .def("save_patch", &ObxfPyEngine::savePatch, py::arg("path"),
             "Save current state as FXP patch.")
        .def("find_patch", &ObxfPyEngine::findPatch, py::arg("name"),
             "Search factory then user patches for a file by name (.fxp extension is optional). "
             "Returns full path or throws.")

        // Audio
        .def("process", &ObxfPyEngine::process, py::arg("n_samples"),
             "Render n_samples. Returns (left, right) as numpy float32 arrays.");
}

} // namespace obxf::python

#endif // OBXF_SRC_OBXFPYTHON_OBXFPYTHON_H