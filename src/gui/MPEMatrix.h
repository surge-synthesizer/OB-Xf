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

/*
 * MPEMatrix.h — temporary developer UI for the MPE modulation matrix.
 *
 * This component uses stock JUCE widgets and is NOT intended for production.
 * It will be replaced with a properly skinned component before release.
 * The control layout and callback wiring may survive into the skinned version.
 */

#ifndef OBXF_SRC_GUI_MPEMATRIX_H
#define OBXF_SRC_GUI_MPEMATRIX_H

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../PluginProcessor.h"
#include "../engine/VoiceMatrix.h"
#include "../parameter/SynthParam.h"

class MPEMatrixEditor : public juce::Component
{
  public:
    explicit MPEMatrixEditor(ObxfAudioProcessor &proc) : processor(proc)
    {
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);

        for (int i = 0; i < numMatrixRows; ++i)
        {
            auto &rc = rowControls[i];

            rc.source = std::make_unique<juce::ComboBox>();
            rc.source->addItem("None", 1);
            rc.source->addItem("Voice Bend", 2);
            rc.source->addItem("Channel Pressure", 3);
            rc.source->addItem("Timbre", 4);
            rc.source->addItem("Velocity", 5);
            rc.source->addItem("Release Velocity", 6);
            rc.source->addItem("LFO 2", 7);
            rc.source->onChange = [this, i] { onRowChanged(i); };
            addAndMakeVisible(*rc.source);

            rc.target = std::make_unique<juce::ComboBox>();
            rc.target->addItem("---", 1);
            rc.target->addItem("Filter Cutoff", 2);
            rc.target->addItem("Filter Resonance", 3);
            rc.target->addItem("Osc 1 Pitch", 4);
            rc.target->addItem("Osc 2 Pitch", 5);
            rc.target->addItem("Osc 2 Detune", 6);
            rc.target->addItem("Osc 2 PW Offset", 7);
            rc.target->addItem("Osc 1 Vol", 8);
            rc.target->addItem("Osc 2 Vol", 9);
            rc.target->addItem("Noise Vol", 10);
            rc.target->addItem("Ring Mod Vol", 11);
            rc.target->addItem("Noise Color", 12);
            rc.target->addItem("Osc Pitch (Both)", 13);
            rc.target->addItem("Unison Detune", 14);
            rc.target->addItem("Osc PW", 15);
            rc.target->addItem("Crossmod", 16);
            rc.target->addItem("LFO1 Mod 1", 17);
            rc.target->addItem("LFO1 Mod 2", 18);
            rc.target->addItem("LFO2 Rate", 19);
            rc.target->addItem("LFO2 Mod 1", 20);
            rc.target->addItem("LFO2 Mod 2", 21);
            rc.target->addItem("Filter Env A", 22);
            rc.target->addItem("Filter Env R", 23);
            rc.target->addItem("Amp Env A", 24);
            rc.target->addItem("Amp Env R", 25);
            rc.target->onChange = [this, i] { onRowChanged(i); };
            addAndMakeVisible(*rc.target);

            rc.depth = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                      juce::Slider::TextBoxRight);
            rc.depth->setRange(-1.0, 1.0, 0.01);
            rc.depth->setDoubleClickReturnValue(true, 0.0);
            rc.depth->onValueChange = [this, i] { onRowChanged(i); };
            addAndMakeVisible(*rc.depth);
        }

        refresh();
    }

    /*
     * Re-reads the current matrix state from the processor and updates all controls.
     * Reading voiceMatrix from the message thread is a benign race for a dev tool.
     */
    void refresh()
    {
        const auto &vm = processor.getSynth().getMotherboard()->voiceMatrix;
        for (int i = 0; i < numMatrixRows; ++i)
            syncRow(i, vm.rows[i]);
    }

    /* Sync UI directly from a known set of rows (e.g. after a preset push). */
    void syncUI(const std::array<MatrixRow, numMatrixRows> &rows)
    {
        for (int i = 0; i < numMatrixRows; ++i)
            syncRow(i, rows[i]);
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black);

        g.setColour(juce::Colour(0xffe0e0d8));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), 4.f, 1.5f);

        g.setColour(juce::Colour(0xffe0e0d8));
        g.setFont(juce::FontOptions(14.f, juce::Font::bold));
        g.drawText("MPE Matrix - dev editor (temporary)", kPad, kPad, getWidth() - kPad * 2,
                   kTitleH, juce::Justification::centred);

        g.setColour(juce::Colour(0xffffcc44));
        g.setFont(juce::FontOptions(11.f));
        g.drawText("MPE support is alpha.  Changes here may not be compatible with 1.1 release.",
                   kPad, kPad + kTitleH + kTitleGap, getWidth() - kPad * 2, kWarningH,
                   juce::Justification::centred);

        g.setColour(juce::Colour(0xff888880));
        g.setFont(juce::FontOptions(11.f));
        g.drawText("Source", sourceX(), kColHeaderY, kSourceW, kColHeaderH,
                   juce::Justification::centredLeft);
        g.drawText("Target", targetX(), kColHeaderY, kTargetW, kColHeaderH,
                   juce::Justification::centredLeft);
        g.drawText("Depth", depthX(), kColHeaderY, kDepthW, kColHeaderH,
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        closeButton.setBounds(getWidth() - kPad - kCloseBtnW, kPad, kCloseBtnW, kTitleH);

        for (int i = 0; i < numMatrixRows; ++i)
        {
            const int y = rowY(i);
            auto &rc = rowControls[i];
            rc.source->setBounds(sourceX(), y, kSourceW, kRowH);
            rc.target->setBounds(targetX(), y, kTargetW, kRowH);
            rc.depth->setBounds(depthX(), y, kDepthW, kRowH);
        }
    }

    static int preferredWidth()
    {
        return kPad + kSourceW + kColGap + kTargetW + kColGap + kDepthW + kPad;
    }
    static int preferredHeight() { return rowY(numMatrixRows - 1) + kRowH + kPad; }

  private:
    ObxfAudioProcessor &processor;
    juce::TextButton closeButton{"Close"};

    struct RowControls
    {
        std::unique_ptr<juce::ComboBox> source;
        std::unique_ptr<juce::ComboBox> target;
        std::unique_ptr<juce::Slider> depth;
    };

    std::array<RowControls, numMatrixRows> rowControls;

    /* Layout constants */
    static constexpr int kCloseBtnW = 52;
    static constexpr int kPad = 12;
    static constexpr int kTitleH = 22;
    static constexpr int kTitleGap = 4;
    static constexpr int kWarningH = 16;
    static constexpr int kWarningGap = 8;
    static constexpr int kColHeaderH = 20;
    static constexpr int kColHeaderGap = 4;
    static constexpr int kRowH = 26;
    static constexpr int kRowGap = 6;
    static constexpr int kSourceW = 130;
    static constexpr int kTargetW = 145;
    static constexpr int kDepthW = 185;
    static constexpr int kColGap = 8;

    static constexpr int kColHeaderY = kPad + kTitleH + kTitleGap + kWarningH + kWarningGap;
    static constexpr int kFirstRowY = kColHeaderY + kColHeaderH + kColHeaderGap;

    static int sourceX() { return kPad; }
    static int targetX() { return sourceX() + kSourceW + kColGap; }
    static int depthX() { return targetX() + kTargetW + kColGap; }
    static int rowY(int i) { return kFirstRowY + i * (kRowH + kRowGap); }

    void syncRow(int i, const MatrixRow &row)
    {
        auto &rc = rowControls[i];
        rc.source->onChange = nullptr;
        rc.target->onChange = nullptr;
        rc.depth->onValueChange = nullptr;

        rc.source->setSelectedId(sourceToId(row.source), juce::dontSendNotification);
        rc.target->setSelectedId(targetToId(row.target), juce::dontSendNotification);
        rc.depth->setValue(row.depth, juce::dontSendNotification);

        rc.source->onChange = [this, i] { onRowChanged(i); };
        rc.target->onChange = [this, i] { onRowChanged(i); };
        rc.depth->onValueChange = [this, i] { onRowChanged(i); };
    }

    void onRowChanged(int idx)
    {
        auto &rc = rowControls[idx];
        MatrixRow row;
        row.source = idToSource(rc.source->getSelectedId());
        row.target = idToTarget(rc.target->getSelectedId());
        row.depth = static_cast<float>(rc.depth->getValue());
        processor.pushMatrixRowUpdate(idx, row);
    }

    static int sourceToId(MatrixSource s)
    {
        switch (s)
        {
        case MatrixSource::VoiceBend:
            return 2;
        case MatrixSource::ChannelPressure:
            return 3;
        case MatrixSource::Timbre:
            return 4;
        case MatrixSource::Velocity:
            return 5;
        case MatrixSource::ReleaseVelocity:
            return 6;
        case MatrixSource::LFO2:
            return 7;
        case MatrixSource::None:
        default:
            return 1;
        }
    }

    static MatrixSource idToSource(int id)
    {
        switch (id)
        {
        case 2:
            return MatrixSource::VoiceBend;
        case 3:
            return MatrixSource::ChannelPressure;
        case 4:
            return MatrixSource::Timbre;
        case 5:
            return MatrixSource::Velocity;
        case 6:
            return MatrixSource::ReleaseVelocity;
        case 7:
            return MatrixSource::LFO2;
        default:
            return MatrixSource::None;
        }
    }

    static int targetToId(const std::string &t)
    {
        if (t == SynthParam::ID::FilterCutoff)
            return 2;
        if (t == SynthParam::ID::FilterResonance)
            return 3;
        if (t == SynthParam::ID::Osc1Pitch)
            return 4;
        if (t == SynthParam::ID::Osc2Pitch)
            return 5;
        if (t == SynthParam::ID::Osc2Detune)
            return 6;
        if (t == SynthParam::ID::Osc2PWOffset)
            return 7;
        if (t == SynthParam::ID::Osc1Vol)
            return 8;
        if (t == SynthParam::ID::Osc2Vol)
            return 9;
        if (t == SynthParam::ID::NoiseVol)
            return 10;
        if (t == SynthParam::ID::RingModVol)
            return 11;
        if (t == SynthParam::ID::NoiseColor)
            return 12;
        if (t == SynthParam::ID::OscPitch)
            return 13;
        if (t == SynthParam::ID::UnisonDetune)
            return 14;
        if (t == SynthParam::ID::OscPW)
            return 15;
        if (t == SynthParam::ID::OscCrossmod)
            return 16;
        if (t == SynthParam::ID::LFO1ModAmount1)
            return 17;
        if (t == SynthParam::ID::LFO1ModAmount2)
            return 18;
        if (t == SynthParam::ID::LFO2Rate)
            return 19;
        if (t == SynthParam::ID::LFO2ModAmount1)
            return 20;
        if (t == SynthParam::ID::LFO2ModAmount2)
            return 21;
        if (t == SynthParam::ID::FilterEnvAttack)
            return 22;
        if (t == SynthParam::ID::FilterEnvRelease)
            return 23;
        if (t == SynthParam::ID::AmpEnvAttack)
            return 24;
        if (t == SynthParam::ID::AmpEnvRelease)
            return 25;
        return 1;
    }

    static std::string idToTarget(int id)
    {
        switch (id)
        {
        case 2:
            return SynthParam::ID::FilterCutoff;
        case 3:
            return SynthParam::ID::FilterResonance;
        case 4:
            return SynthParam::ID::Osc1Pitch;
        case 5:
            return SynthParam::ID::Osc2Pitch;
        case 6:
            return SynthParam::ID::Osc2Detune;
        case 7:
            return SynthParam::ID::Osc2PWOffset;
        case 8:
            return SynthParam::ID::Osc1Vol;
        case 9:
            return SynthParam::ID::Osc2Vol;
        case 10:
            return SynthParam::ID::NoiseVol;
        case 11:
            return SynthParam::ID::RingModVol;
        case 12:
            return SynthParam::ID::NoiseColor;
        case 13:
            return SynthParam::ID::OscPitch;
        case 14:
            return SynthParam::ID::UnisonDetune;
        case 15:
            return SynthParam::ID::OscPW;
        case 16:
            return SynthParam::ID::OscCrossmod;
        case 17:
            return SynthParam::ID::LFO1ModAmount1;
        case 18:
            return SynthParam::ID::LFO1ModAmount2;
        case 19:
            return SynthParam::ID::LFO2Rate;
        case 20:
            return SynthParam::ID::LFO2ModAmount1;
        case 21:
            return SynthParam::ID::LFO2ModAmount2;
        case 22:
            return SynthParam::ID::FilterEnvAttack;
        case 23:
            return SynthParam::ID::FilterEnvRelease;
        case 24:
            return SynthParam::ID::AmpEnvAttack;
        case 25:
            return SynthParam::ID::AmpEnvRelease;
        default:
            return {};
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEMatrixEditor)
};

#endif // OBXF_SRC_GUI_MPEMATRIX_H
