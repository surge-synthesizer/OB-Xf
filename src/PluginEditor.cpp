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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "gui/ImageButton.h"
#include "Utils.h"

static std::weak_ptr<OBXFJUCELookAndFeel> sharedLookAndFeelWeak;

//==============================================================================
ObxfAudioProcessorEditor::ObxfAudioProcessorEditor(ObxfAudioProcessor &p)
    : AudioProcessorEditor(&p), ScalableComponent(&p), processor(p), utils(p.getUtils()),
      paramManager(p.getParamManager()), skinFolder(utils.getSkinFolder()), sizeStart(4000),
      presetStart(3000), bankStart(2000), skinStart(1000), skins(utils.getSkinFiles()),
      banks(utils.getBankFiles())

{
    {
        if (const auto sp = sharedLookAndFeelWeak.lock())
        {
            lookAndFeelPtr = sp;
        }
        else
        {
            lookAndFeelPtr = std::make_shared<OBXFJUCELookAndFeel>();
            sharedLookAndFeelWeak = lookAndFeelPtr;
            juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeelPtr.get());
        }
    }

    keyCommandHandler = std::make_unique<KeyCommandHandler>();
    keyCommandHandler->setNextProgramCallback([this]() {
        nextProgram();
        grabKeyboardFocus();
    });
    keyCommandHandler->setPrevProgramCallback([this]() {
        prevProgram();
        grabKeyboardFocus();
    });

    commandManager.registerAllCommandsForTarget(keyCommandHandler.get());
    commandManager.setFirstCommandTarget(keyCommandHandler.get());
    commandManager.getKeyMappings()->resetToDefaultMappings();
    getTopLevelComponent()->addKeyListener(commandManager.getKeyMappings());

    aboutScreen = std::make_unique<AboutScreen>();
    addChildComponent(*aboutScreen);

    if (juce::PluginHostType().isProTools())
    {
    }
    else
    {
        startTimer(100);
    }; // Fix ProTools file dialog focus issues

    loadSkin(processor);

    const int initialWidth = backgroundImage.getWidth();
    const int initialHeight = backgroundImage.getHeight();

    setSize(initialWidth, initialHeight);

    setOriginalBounds(getLocalBounds());

    setResizable(true, false);

    constrainer = std::make_unique<AspectRatioDownscaleConstrainer>(initialWidth, initialHeight);
    constrainer->setMinimumSize(initialWidth / 4, initialHeight / 4);
    setConstrainer(constrainer.get());

    updateFromHost();
}

void ObxfAudioProcessorEditor::resized()
{
    if (presetBar != nullptr)
    {
        const int presetBarWidth = presetBar->getWidth();
        const int presetBarHeight = presetBar->getHeight();
        const int xPos = (getWidth() - presetBarWidth) / 2;
        const int yPos = getHeight() - presetBarHeight;

        presetBar->setBounds(xPos, yPos, presetBarWidth, presetBarHeight);
    }

    if (setPresetNameWindow != nullptr)
    {
        if (const auto wrapper =
                dynamic_cast<ObxfAudioProcessorEditor *>(processor.getActiveEditor()))
        {
            const auto w = proportionOfWidth(0.25f);
            const auto h = proportionOfHeight(0.3f);
            const auto x = proportionOfWidth(0.5f) - (w / 2);
            auto y = wrapper->getY();

            if (setPresetNameWindow != nullptr)
            {
                y += proportionOfHeight(0.15f);
                setPresetNameWindow->setBounds(x, y, w, h);
            }
        }
    }

    skinFolder = utils.getCurrentSkinFolder();
    const juce::File coords = skinFolder.getChildFile("coords.xml");
    if (!coords.existsAsFile())
    {
        return;
    }
    juce::XmlDocument skin(coords);
    if (const auto doc = skin.getDocumentElement())
    {
        if (doc->getTagName() == "PROPERTIES")
        {
            for (const auto *child : doc->getChildWithTagNameIterator("VALUE"))
            {

                juce::String name = child->getStringAttribute("NAME");
                const int x = child->getIntAttribute("x");
                const int y = child->getIntAttribute("y");
                const int d = child->getIntAttribute("d");
                const int w = child->getIntAttribute("w");
                const int h = child->getIntAttribute("h");

                if (mappingComps[name] != nullptr)
                {
                    if (auto *knob = dynamic_cast<Knob *>(mappingComps[name]))
                    {
                        knob->setBounds(transformBounds(x, y, d, d));
                        knob->setPopupDisplayEnabled(true, true, knob->getParentComponent());
                    }
                    else if (dynamic_cast<ButtonList *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }

                    else if (dynamic_cast<ToggleButton *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }

                    else if (dynamic_cast<ImageMenu *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, d, d));
                    }
                    else if (dynamic_cast<juce::ImageButton *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<MidiKeyboard *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }
                }
            }
        }
    }
}

void ObxfAudioProcessorEditor::loadSkin(ObxfAudioProcessor &ownerFilter)
{
    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    imageButtons.clear();
    popupMenus.clear();
    mappingComps.clear();
    ownerFilter.removeChangeListener(this);

    skinFolder = utils.getCurrentSkinFolder();
    const juce::File coords = skinFolder.getChildFile("coords.xml");
    if (const bool useClassicSkin = coords.existsAsFile(); !useClassicSkin)
    {
        addMenuButton(14, 25, 20, "menu");
        rebuildComponents(processor);
        return;
    }

    juce::XmlDocument skin(coords);
    if (const auto doc = skin.getDocumentElement(); !doc)
    {
        notLoadSkin = true;
        setSize(1440, 486);
    }
    else
    {
        if (doc->getTagName() == "PROPERTIES")
        {
            using namespace SynthParam;

            for (const auto *child : doc->getChildWithTagNameIterator("VALUE"))
            {

                juce::String name = child->getStringAttribute("NAME");
                const int x = child->getIntAttribute("x");
                const int y = child->getIntAttribute("y");
                const int d = child->getIntAttribute("d");
                const int w = child->getIntAttribute("w");
                const int h = child->getIntAttribute("h");

                if (name == "midiKeyboard")
                {
                    const auto keyboard = addMidiKeyboard(x, y, w, h);
                    mappingComps["midiKeyboard"] = keyboard;
                }

                if (name == "legatoSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, LEGATOMODE, Name::LegatoMode,
                                            "legato");
                        list != nullptr)
                    {
                        legatoSwitch = std::move(list);
                        mappingComps["legatoSwitch"] = legatoSwitch.get();
                    }
                }

                if (name == "voiceSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, VOICE_COUNT, Name::VoiceCount,
                                            "voices");
                        list != nullptr)
                    {
                        voiceSwitch = std::move(list);
                        mappingComps["voiceSwitch"] = voiceSwitch.get();
                    }
                }

                if (name == "bendUpRangeSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, PITCH_BEND_UP,
                                            Name::PitchBendUpRange, "bendrange");
                        list != nullptr)
                    {
                        bendUpRangeSwitch = std::move(list);
                        mappingComps["bendUpRangeSwitch"] = bendUpRangeSwitch.get();
                    }
                }

                if (name == "bendDownRangeSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, PITCH_BEND_DOWN,
                                            Name::PitchBendDownRange, "bendrange");
                        list != nullptr)
                    {
                        bendDownRangeSwitch = std::move(list);
                        mappingComps["bendDownRangeSwitch"] = bendDownRangeSwitch.get();
                    }
                }

                if (name == "menu")
                {
                    const auto imageButton = addMenuButton(x, y, d, "menu");
                    mappingComps["menu"] = imageButton;
                }

                if (name == "resonanceKnob")
                {
                    resonanceKnob = addKnob(x, y, d, ownerFilter, RESONANCE, 0.f, Name::Resonance);
                    mappingComps["resonanceKnob"] = resonanceKnob.get();
                }
                if (name == "cutoffKnob")
                {
                    cutoffKnob = addKnob(x, y, d, ownerFilter, CUTOFF, 1.f, Name::Cutoff);
                    mappingComps["cutoffKnob"] = cutoffKnob.get();
                }
                if (name == "filterEnvelopeAmtKnob")
                {
                    filterEnvelopeAmtKnob =
                        addKnob(x, y, d, ownerFilter, ENVELOPE_AMT, 0.f, Name::FilterEnvAmount);
                    mappingComps["filterEnvelopeAmtKnob"] = filterEnvelopeAmtKnob.get();
                }
                if (name == "multimodeKnob")
                {
                    multimodeKnob = addKnob(x, y, d, ownerFilter, MULTIMODE, 0.f, Name::Multimode);
                    mappingComps["multimodeKnob"] = multimodeKnob.get();
                }

                if (name == "volumeKnob")
                {
                    volumeKnob = addKnob(x, y, d, ownerFilter, VOLUME, 0.5f, Name::Volume);
                    mappingComps["volumeKnob"] = volumeKnob.get();
                }
                if (name == "portamentoKnob")
                {
                    portamentoKnob =
                        addKnob(x, y, d, ownerFilter, PORTAMENTO, 0.f, Name::Portamento);
                    mappingComps["portamentoKnob"] = portamentoKnob.get();
                }
                if (name == "osc1PitchKnob")
                {
                    osc1PitchKnob = addKnob(x, y, d, ownerFilter, OSC1P, 0.5f, Name::Osc1Pitch);
                    osc1PitchKnob->cmdDragCallback = [](const double value) {
                        const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
                        return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0,
                                          1.0);
                    };
                    osc1PitchKnob->altDragCallback = [](const double value) {
                        const auto octValue = (int)juce::jmap(value, -2.0, 2.0);
                        return juce::jmap((double)octValue, -2.0, 2.0, 0.0, 1.0);
                    };
                    mappingComps["osc1PitchKnob"] = osc1PitchKnob.get();
                }
                if (name == "pulseWidthKnob")
                {
                    pulseWidthKnob = addKnob(x, y, d, ownerFilter, PW, 0.f, Name::PulseWidth);
                    mappingComps["pulseWidthKnob"] = pulseWidthKnob.get();
                }
                if (name == "osc2PitchKnob")
                {
                    osc2PitchKnob = addKnob(x, y, d, ownerFilter, OSC2P, 0.5f, Name::Osc2Pitch);
                    osc2PitchKnob->cmdDragCallback = [](const double value) {
                        const auto semitoneValue = (int)juce::jmap(value, -24.0, 24.0);
                        return juce::jmap((double)semitoneValue, -24.0, 24.0, 0.0, 1.0);
                    };
                    osc2PitchKnob->altDragCallback = [](const double value) {
                        const auto octValue = (int)juce::jmap(value, -2.0, 2.0);
                        return juce::jmap((double)octValue, -2.0, 2.0, 0.0, 1.0);
                    };
                    mappingComps["osc2PitchKnob"] = osc2PitchKnob.get();
                }
                //
                if (name == "osc1MixKnob")
                {
                    osc1MixKnob = addKnob(x, y, d, ownerFilter, OSC1MIX, 1.f, Name::Osc1Mix);
                    mappingComps["osc1MixKnob"] = osc1MixKnob.get();
                }
                if (name == "osc2MixKnob")
                {
                    osc2MixKnob = addKnob(x, y, d, ownerFilter, OSC2MIX, 1.f, Name::Osc2Mix);
                    mappingComps["osc2MixKnob"] = osc2MixKnob.get();
                }
                if (name == "noiseMixKnob")
                {
                    noiseMixKnob = addKnob(x, y, d, ownerFilter, NOISEMIX, 0.f, Name::NoiseMix);
                    mappingComps["noiseMixKnob"] = noiseMixKnob.get();
                }

                if (name == "ringModMixKnob")
                {
                    ringModMixKnob =
                        addKnob(x, y, d, ownerFilter, RINGMODMIX, 0.f, Name::RingModMix);
                    mappingComps["ringModMixKnob"] = ringModMixKnob.get();
                }

                if (name == "xmodKnob")
                {
                    xmodKnob = addKnob(x, y, d, ownerFilter, XMOD, 0.f, Name::Xmod);
                    mappingComps["xmodKnob"] = xmodKnob.get();
                }
                if (name == "osc2DetuneKnob")
                {
                    osc2DetuneKnob =
                        addKnob(x, y, d, ownerFilter, OSC2_DET, 0.f, Name::Oscillator2Detune);
                    mappingComps["osc2DetuneKnob"] = osc2DetuneKnob.get();
                }

                if (name == "envPitchModKnob")
                {
                    envPitchModKnob =
                        addKnob(x, y, d, ownerFilter, ENVPITCH, 0.f, Name::EnvelopeToPitch);
                    mappingComps["envPitchModKnob"] = envPitchModKnob.get();
                }
                if (name == "brightnessKnob")
                {
                    brightnessKnob =
                        addKnob(x, y, d, ownerFilter, BRIGHTNESS, 1.f, Name::Brightness);
                    mappingComps["brightnessKnob"] = brightnessKnob.get();
                }

                if (name == "attackKnob")
                {
                    attackKnob = addKnob(x, y, d, ownerFilter, LATK, 0.f, Name::Attack);
                    mappingComps["attackKnob"] = attackKnob.get();
                }
                if (name == "decayKnob")
                {
                    decayKnob = addKnob(x, y, d, ownerFilter, LDEC, 0.f, Name::Decay);
                    mappingComps["decayKnob"] = decayKnob.get();
                }
                if (name == "sustainKnob")
                {
                    sustainKnob = addKnob(x, y, d, ownerFilter, LSUS, 1.f, Name::Sustain);
                    mappingComps["sustainKnob"] = sustainKnob.get();
                }
                if (name == "releaseKnob")
                {
                    releaseKnob = addKnob(x, y, d, ownerFilter, LREL, 0.f, Name::Release);
                    mappingComps["releaseKnob"] = releaseKnob.get();
                }

                if (name == "fattackKnob")
                {
                    fattackKnob = addKnob(x, y, d, ownerFilter, FATK, 0.f, Name::FilterAttack);
                    mappingComps["fattackKnob"] = fattackKnob.get();
                }
                if (name == "fdecayKnob")
                {
                    fdecayKnob = addKnob(x, y, d, ownerFilter, FDEC, 0.f, Name::FilterDecay);
                    mappingComps["fdecayKnob"] = fdecayKnob.get();
                }
                if (name == "fsustainKnob")
                {
                    fsustainKnob = addKnob(x, y, d, ownerFilter, FSUS, 1.f, Name::FilterSustain);
                    mappingComps["fsustainKnob"] = fsustainKnob.get();
                }
                if (name == "freleaseKnob")
                {
                    freleaseKnob = addKnob(x, y, d, ownerFilter, FREL, 0.f, Name::FilterRelease);
                    mappingComps["freleaseKnob"] = freleaseKnob.get();
                }

                if (name == "lfoFrequencyKnob")
                {
                    lfoFrequencyKnob =
                        addKnob(x, y, d, ownerFilter, LFOFREQ, 0.5f, Name::LfoFrequency); // 4 Hz
                    mappingComps["lfoFrequencyKnob"] = lfoFrequencyKnob.get();
                }
                if (name == "lfoAmt1Knob")
                {
                    lfoAmt1Knob = addKnob(x, y, d, ownerFilter, LFO1AMT, 0.f, Name::LfoAmount1);
                    mappingComps["lfoAmt1Knob"] = lfoAmt1Knob.get();
                }
                if (name == "lfoAmt2Knob")
                {
                    lfoAmt2Knob = addKnob(x, y, d, ownerFilter, LFO2AMT, 0.f, Name::LfoAmount2);
                    mappingComps["lfoAmt2Knob"] = lfoAmt2Knob.get();
                }

                if (name == "lfoWave1Knob")
                {
                    lfoWave1Knob =
                        addKnob(x, y, d, ownerFilter, LFOSINWAVE, 0.5f, Name::LfoSineWave);
                    mappingComps["lfoWave1Knob"] = lfoWave1Knob.get();
                }
                if (name == "lfoWave2Knob")
                {
                    lfoWave2Knob =
                        addKnob(x, y, d, ownerFilter, LFOSQUAREWAVE, 0.5f, Name::LfoSquareWave);
                    mappingComps["lfoWave2Knob"] = lfoWave2Knob.get();
                }
                if (name == "lfoWave3Knob")
                {
                    lfoWave3Knob =
                        addKnob(x, y, d, ownerFilter, LFOSHWAVE, 0.5f, Name::LfoSampleHoldWave);
                    mappingComps["lfoWave3Knob"] = lfoWave3Knob.get();
                }

                if (name == "lfoOsc1Button")
                {
                    lfoOsc1Button = addButton(x, y, w, h, ownerFilter, LFOOSC1, Name::LfoOsc1);
                    mappingComps["lfoOsc1Button"] = lfoOsc1Button.get();
                }
                if (name == "lfoOsc2Button")
                {
                    lfoOsc2Button = addButton(x, y, w, h, ownerFilter, LFOOSC2, Name::LfoOsc2);
                    mappingComps["lfoOsc2Button"] = lfoOsc2Button.get();
                }
                if (name == "lfoFilterButton")
                {
                    lfoFilterButton =
                        addButton(x, y, w, h, ownerFilter, LFOFILTER, Name::LfoFilter);
                    mappingComps["lfoFilterButton"] = lfoFilterButton.get();
                }

                if (name == "lfoPwm1Button")
                {
                    lfoPwm1Button = addButton(x, y, w, h, ownerFilter, LFOPW1, Name::LfoPw1);
                    mappingComps["lfoPwm1Button"] = lfoPwm1Button.get();
                }
                if (name == "lfoPwm2Button")
                {
                    lfoPwm2Button = addButton(x, y, w, h, ownerFilter, LFOPW2, Name::LfoPw2);
                    mappingComps["lfoPwm2Button"] = lfoPwm2Button.get();
                }
                if (name == "lfoVolumeButton")
                {
                    lfoVolumeButton =
                        addButton(x, y, w, h, ownerFilter, LFOVOLUME, Name::LfoVolume);
                    mappingComps["lfoVolumeButton"] = lfoVolumeButton.get();
                }

                if (name == "hardSyncButton")
                {
                    hardSyncButton = addButton(x, y, w, h, ownerFilter, OSC2HS, Name::Osc2HardSync);
                    mappingComps["hardSyncButton"] = hardSyncButton.get();
                }
                if (name == "osc1SawButton")
                {
                    osc1SawButton = addButton(x, y, w, h, ownerFilter, OSC1Saw, Name::Osc1Saw);
                    mappingComps["osc1SawButton"] = osc1SawButton.get();
                }
                if (name == "osc2SawButton")
                {
                    osc2SawButton = addButton(x, y, w, h, ownerFilter, OSC2Saw, Name::Osc2Saw);
                    mappingComps["osc2SawButton"] = osc2SawButton.get();
                }

                if (name == "osc1PulButton")
                {
                    osc1PulButton = addButton(x, y, w, h, ownerFilter, OSC1Pul, Name::Osc1Pulse);
                    mappingComps["osc1PulButton"] = osc1PulButton.get();
                }
                if (name == "osc2PulButton")
                {
                    osc2PulButton = addButton(x, y, w, h, ownerFilter, OSC2Pul, Name::Osc2Pulse);
                    mappingComps["osc2PulButton"] = osc2PulButton.get();
                }

                if (name == "pitchEnvInvertButton")
                {
                    pitchEnvInvertButton =
                        addButton(x, y, w, h, ownerFilter, ENVPITCHINV, Name::EnvelopeToPitchInv);
                    mappingComps["pitchEnvInvertButton"] = pitchEnvInvertButton.get();
                }

                if (name == "pwEnvInvertButton")
                {
                    pwEnvInvertButton =
                        addButton(x, y, w, h, ownerFilter, ENVPWINV, Name::EnvelopeToPWInv);
                    mappingComps["pwEnvInvertButton"] = pwEnvInvertButton.get();
                }

                if (name == "filterBPBlendButton")
                {
                    filterBPBlendButton =
                        addButton(x, y, w, h, ownerFilter, BANDPASS, Name::BandpassBlend);
                    mappingComps["filterBPBlendButton"] = filterBPBlendButton.get();
                }
                if (name == "fourPoleButton")
                {
                    fourPoleButton = addButton(x, y, w, h, ownerFilter, FOURPOLE, Name::FourPole);
                    mappingComps["fourPoleButton"] = fourPoleButton.get();
                }
                if (name == "filterHQButton")
                {
                    filterHQButton =
                        addButton(x, y, w, h, ownerFilter, FILTER_WARM, Name::FilterWarm);
                    mappingComps["filterHQButton"] = filterHQButton.get();
                }

                if (name == "filterKeyFollowKnob")
                {
                    filterKeyFollowKnob =
                        addKnob(x, y, d, ownerFilter, FLT_KF, 0.f, Name::FilterKeyFollow);
                    mappingComps["filterKeyFollowKnob"] = filterKeyFollowKnob.get();
                }

                if (name == "unisonButton")
                {
                    unisonButton = addButton(x, y, w, h, ownerFilter, UNISON, Name::Unison);
                    mappingComps["unisonButton"] = unisonButton.get();
                }

                if (name == "tuneKnob")
                {
                    tuneKnob = addKnob(x, y, d, ownerFilter, TUNE, 0.5f, Name::Tune);
                    mappingComps["tuneKnob"] = tuneKnob.get();
                }
                if (name == "transposeKnob")
                {
                    transposeKnob = addKnob(x, y, d, ownerFilter, OCTAVE, 0.5f, Name::Transpose);
                    mappingComps["transposeKnob"] = transposeKnob.get();
                }

                if (name == "voiceDetuneKnob")
                {
                    voiceDetuneKnob = addKnob(x, y, d, ownerFilter, UDET, 0.25f, Name::VoiceDetune);
                    mappingComps["voiceDetuneKnob"] = voiceDetuneKnob.get();
                }

                if (name == "vibratoRateKnob")
                {
                    vibratoRateKnob = addKnob(x, y, d, ownerFilter, BENDLFORATE, 0.2f,
                                              Name::VibratoRate); // 4 Hz
                    mappingComps["vibratoRateKnob"] = vibratoRateKnob.get();
                }
                if (name == "veloFltEnvKnob")
                {
                    veloFltEnvKnob = addKnob(x, y, d, ownerFilter, VFLTENV, 0.f, Name::VFltFactor);
                    mappingComps["veloFltEnvKnob"] = veloFltEnvKnob.get();
                }
                if (name == "veloAmpEnvKnob")
                {
                    veloAmpEnvKnob = addKnob(x, y, d, ownerFilter, VAMPENV, 0.f, Name::VAmpFactor);
                    mappingComps["veloAmpEnvKnob"] = veloAmpEnvKnob.get();
                }
                if (name == "midiLearnButton")
                {
                    midiLearnButton = addButton(x, y, w, h, ownerFilter, -1, Name::MidiLearn);
                    mappingComps["midiLearnButton"] = midiLearnButton.get();
                    midiLearnButton->onClick = [this]() {
                        const bool state = midiLearnButton->getToggleState();
                        paramManager.midiLearnAttachment.set(state);
                    };
                }
                if (name == "midiUnlearnButton")
                {
                    midiUnlearnButton = addButton(x, y, w, h, ownerFilter, -1, Name::MidiUnlearn);
                    mappingComps["midiUnlearnButton"] = midiUnlearnButton.get();
                    midiUnlearnButton->onClick = [this]() {
                        const bool state = midiUnlearnButton->getToggleState();
                        paramManager.midiUnlearnAttachment.set(state);
                    };
                }

                if (name == "pan1Knob")
                {
                    pan1Knob = addKnob(x, y, d, ownerFilter, PAN1, 0.5f, Name::Pan1);
                    pan1Knob->addActionListener(this);
                    mappingComps["pan1Knob"] = pan1Knob.get();
                }
                if (name == "pan2Knob")
                {
                    pan2Knob = addKnob(x, y, d, ownerFilter, PAN2, 0.5f, Name::Pan2);
                    pan2Knob->addActionListener(this);
                    mappingComps["pan2Knob"] = pan2Knob.get();
                }
                if (name == "pan3Knob")
                {
                    pan3Knob = addKnob(x, y, d, ownerFilter, PAN3, 0.5f, Name::Pan3);
                    pan3Knob->addActionListener(this);
                    mappingComps["pan3Knob"] = pan3Knob.get();
                }
                if (name == "pan4Knob")
                {
                    pan4Knob = addKnob(x, y, d, ownerFilter, PAN4, 0.5f, Name::Pan4);
                    pan4Knob->addActionListener(this);
                    mappingComps["pan4Knob"] = pan4Knob.get();
                }

                if (name == "pan5Knob")
                {
                    pan5Knob = addKnob(x, y, d, ownerFilter, PAN5, 0.5f, Name::Pan5);
                    pan5Knob->addActionListener(this);
                    mappingComps["pan5Knob"] = pan5Knob.get();
                }
                if (name == "pan6Knob")
                {
                    pan6Knob = addKnob(x, y, d, ownerFilter, PAN6, 0.5f, Name::Pan6);
                    pan6Knob->addActionListener(this);
                    mappingComps["pan6Knob"] = pan6Knob.get();
                }
                if (name == "pan7Knob")
                {
                    pan7Knob = addKnob(x, y, d, ownerFilter, PAN7, 0.5f, Name::Pan7);
                    pan7Knob->addActionListener(this);
                    mappingComps["pan7Knob"] = pan7Knob.get();
                }
                if (name == "pan8Knob")
                {
                    pan8Knob = addKnob(x, y, d, ownerFilter, PAN8, 0.5f, Name::Pan8);
                    pan8Knob->addActionListener(this);
                    mappingComps["pan8Knob"] = pan8Knob.get();
                }

                if (name == "bendOsc2OnlyButton")
                {
                    bendOsc2OnlyButton =
                        addButton(x, y, w, h, ownerFilter, BENDOSC2, Name::BendOsc2Only);
                    mappingComps["bendOsc2OnlyButton"] = bendOsc2OnlyButton.get();
                }

                if (name == "asPlayedAllocButton")
                {
                    asPlayedAllocButton = addButton(x, y, w, h, ownerFilter, ASPLAYEDALLOCATION,
                                                    Name::AsPlayedAllocation);
                    mappingComps["asPlayedAllocButton"] = asPlayedAllocButton.get();
                }

                if (name == "filterDetuneKnob")
                {
                    filterDetuneKnob =
                        addKnob(x, y, d, ownerFilter, FILTERDER, 0.25f, Name::FilterDetune);
                    mappingComps["filterDetuneKnob"] = filterDetuneKnob.get();
                }
                if (name == "portamentoDetuneKnob")
                {
                    portamentoDetuneKnob =
                        addKnob(x, y, d, ownerFilter, PORTADER, 0.25f, Name::PortamentoDetune);
                    mappingComps["portamentoDetuneKnob"] = portamentoDetuneKnob.get();
                }
                if (name == "envelopeDetuneKnob")
                {
                    envelopeDetuneKnob =
                        addKnob(x, y, d, ownerFilter, ENVDER, 0.25f, Name::EnvelopeDetune);
                    mappingComps["envelopeDetuneKnob"] = envelopeDetuneKnob.get();
                }
                if (name == "volumeDetuneKnob")
                {
                    volumeDetuneKnob =
                        addKnob(x, y, d, ownerFilter, LEVEL_DIF, 0.25f, Name::LevelDetune);
                    mappingComps["volumeDetuneKnob"] = volumeDetuneKnob.get();
                }
                if (name == "lfoSyncButton")
                {
                    lfoSyncButton = addButton(x, y, w, h, ownerFilter, LFO_SYNC, Name::LfoSync);
                    mappingComps["lfoSyncButton"] = lfoSyncButton.get();
                }
                if (name == "pwEnvKnob")
                {
                    pwEnvKnob = addKnob(x, y, d, ownerFilter, PW_ENV, 0.f, Name::PwEnv);
                    mappingComps["pwEnvKnob"] = pwEnvKnob.get();
                }
                if (name == "pwEnvBothButton")
                {
                    pwEnvBothButton =
                        addButton(x, y, w, h, ownerFilter, PW_ENV_BOTH, Name::PwEnvBoth);
                    mappingComps["pwEnvBothButton"] = pwEnvBothButton.get();
                }
                if (name == "envPitchBothButton")
                {
                    envPitchBothButton =
                        addButton(x, y, w, h, ownerFilter, ENV_PITCH_BOTH, Name::EnvPitchBoth);
                    mappingComps["envPitchBothButton"] = envPitchBothButton.get();
                }
                if (name == "fenvInvertButton")
                {
                    fenvInvertButton =
                        addButton(x, y, w, h, ownerFilter, FENV_INVERT, Name::FenvInvert);
                    mappingComps["fenvInvertButton"] = fenvInvertButton.get();
                }
                if (name == "pwOffsetKnob")
                {
                    pwOffsetKnob = addKnob(x, y, d, ownerFilter, PW_OSC2_OFS, 0.f, Name::PwOsc2Ofs);
                    mappingComps["pwOffsetKnob"] = pwOffsetKnob.get();
                }
                if (name == "selfOscPushButton")
                {
                    selfOscPushButton =
                        addButton(x, y, w, h, ownerFilter, SELF_OSC_PUSH, Name::SelfOscPush);
                    mappingComps["selfOscPushButton"] = selfOscPushButton.get();
                }
            }
        }

        presetBar = std::make_unique<PresetBar>(*this);
        addAndMakeVisible(*presetBar);
        presetBar->leftClicked = [this](juce::Point<int> &pos) {
            const uint8_t NUM_COLUMNS = 4;

            juce::PopupMenu menu;

            for (int i = 0; i < processor.getNumPrograms(); ++i)
            {
                if (i > 0 && i % (processor.getNumPrograms() / NUM_COLUMNS) == 0)
                {
                    menu.addColumnBreak();
                }

                menu.addItem(i + presetStart + 1,
                             juce::String{i + 1} + ": " + processor.getProgramName(i), true,
                             i == processor.getCurrentProgram());
            }

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                                   juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1)),
                               [this](int result) {
                                   if (result >= (presetStart + 1) &&
                                       result <= (presetStart + processor.getNumPrograms()))
                                   {
                                       result -= 1;
                                       result -= presetStart;
                                       processor.setCurrentProgram(result);
                                   }
                               });
        };
        resized();
    }

    // Prepare data
    if (voiceSwitch)
    {
        auto *menu = voiceSwitch->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            if (i > 1 && ((1 - i) % (MAX_VOICES / NUM_COLUMNS) == 0))
            {
                menu->addColumnBreak();
            }

            voiceSwitch->addChoice(juce::String(i));
        }

        const auto voiceOption = ownerFilter.getValueTreeState()
                                     .getParameter(paramManager.getEngineParameterId(VOICE_COUNT))
                                     ->getValue();
        voiceSwitch->setScrollWheelEnabled(true);
        voiceSwitch->setValue(voiceOption, juce::dontSendNotification);
    }

    if (legatoSwitch)
    {
        legatoSwitch->addChoice("Keep All");
        legatoSwitch->addChoice("Keep Filter Envelope");
        legatoSwitch->addChoice("Keep Amplitude Envelope");
        legatoSwitch->addChoice("Retrig");
        const auto legatoOption = ownerFilter.getValueTreeState()
                                      .getParameter(paramManager.getEngineParameterId(LEGATOMODE))
                                      ->getValue();
        legatoSwitch->setScrollWheelEnabled(true);
        legatoSwitch->setValue(legatoOption, juce::dontSendNotification);
    }

    if (bendUpRangeSwitch)
    {
        auto *menu = bendUpRangeSwitch->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendUpRangeSwitch->addChoice(juce::String(i));
        }

        const auto voiceOption = ownerFilter.getValueTreeState()
                                     .getParameter(paramManager.getEngineParameterId(PITCH_BEND_UP))
                                     ->getValue();
        bendUpRangeSwitch->setScrollWheelEnabled(true);
        bendUpRangeSwitch->setValue(voiceOption, juce::dontSendNotification);
    }

    if (bendDownRangeSwitch)
    {
        auto *menu = bendDownRangeSwitch->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendDownRangeSwitch->addChoice(juce::String(i));
        }

        const auto voiceOption =
            ownerFilter.getValueTreeState()
                .getParameter(paramManager.getEngineParameterId(PITCH_BEND_DOWN))
                ->getValue();
        bendDownRangeSwitch->setScrollWheelEnabled(true);
        bendDownRangeSwitch->setValue(voiceOption, juce::dontSendNotification);
    }

    createMenu();

    ownerFilter.addChangeListener(this);

    scaleFactorChanged();
    repaint();
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    imageButtons.clear();
    popupMenus.clear();
    mappingComps.clear();

    juce::PopupMenu::dismissAllActiveMenus();
}

void ObxfAudioProcessorEditor::scaleFactorChanged()
{
    backgroundImage = getScaledImageFromCache("main");
    presetBar->setBounds((getWidth() - presetBar->getWidth()) / 2,
                         getHeight() - presetBar->getHeight(), presetBar->getWidth(),
                         presetBar->getHeight());

    resized();
}

void ObxfAudioProcessorEditor::placeLabel(const int x, const int y, const juce::String &text)
{
    auto *lab = new juce::Label();
    lab->setBounds(x, y, 110, 20);
    lab->setJustificationType(juce::Justification::centred);
    lab->setText(text, juce::dontSendNotification);
    lab->setInterceptsMouseClicks(false, true);
    addAndMakeVisible(lab);
}

std::unique_ptr<ButtonList>
ObxfAudioProcessorEditor::addList(const int x, const int y, const int w, const int h,
                                  ObxfAudioProcessor &filter, const int parameter,
                                  const juce::String &name, const juce::String &nameImg)
{
#if JUCE_WINDOWS || JUCE_LINUX
    auto *bl = new ButtonList((nameImg), h, &processor);
#else
    auto *bl = new ButtonList(nameImg, h, &processor);
#endif

    buttonListAttachments.add(new ButtonList::ButtonListAttachment(
        filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *bl));

    bl->setBounds(x, y, w, h);
    bl->setTitle(name);
    addAndMakeVisible(bl);

    return std::unique_ptr<ButtonList>(bl);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(const int x, const int y, const int d,
                                                        ObxfAudioProcessor &filter,
                                                        const int parameter, const float defval,
                                                        const juce::String &name,
                                                        const juce::String &asset)
{
    const auto knob = new Knob(asset, 48, &processor);

    knobAttachments.add(new Knob::KnobAttachment(
        filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *knob));

    knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob->setTextBoxStyle(Knob::NoTextBox, true, 0, 0);
    knob->setRange(0, 1);
    knob->setBounds(x, y, d + (d / 6), d + (d / 6));
    knob->setTextBoxIsEditable(false);
    knob->setDoubleClickReturnValue(true, defval, juce::ModifierKeys::noModifiers);
    knob->setTitle(name);
    knob->setValue(filter.getValueTreeState()
                       .getParameter(paramManager.getEngineParameterId(parameter))
                       ->getValue());
    addAndMakeVisible(knob);

    return std::unique_ptr<Knob>(knob);
}

void ObxfAudioProcessorEditor::clean() { this->removeAllChildren(); }

std::unique_ptr<ToggleButton> ObxfAudioProcessorEditor::addButton(const int x, const int y,
                                                                  const int w, const int h,
                                                                  ObxfAudioProcessor &filter,
                                                                  const int parameter,
                                                                  const juce::String &name)
{
    auto *button = new ToggleButton("button", &processor);

    if (parameter > -1)
    {
        toggleAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(
            filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *button));
        button->setToggleState(filter.getValueTreeState()
                                   .getParameter(paramManager.getEngineParameterId(parameter))
                                   ->getValue(),
                               juce::dontSendNotification);
    }
    else
    {
        button->addListener(this);
        button->setToggleState(false, juce::dontSendNotification);
    }

    button->setBounds(x, y, w, h);
    button->setButtonText(name);

    addAndMakeVisible(button);

    return std::unique_ptr<ToggleButton>(button);
}

void ObxfAudioProcessorEditor::actionListenerCallback(const juce::String & /*message*/) {}

juce::Rectangle<int> ObxfAudioProcessorEditor::transformBounds(int x, int y, int w, int h) const
{
    if (originalBounds.isEmpty())
        return {x, y, w, h};

    const int effectiveHeight = getHeight() - presetBar->getHeight();
    const float scaleX = getWidth() / static_cast<float>(originalBounds.getWidth());
    const float scaleY = effectiveHeight / static_cast<float>(originalBounds.getHeight());

    return {juce::roundToInt(x * scaleX), juce::roundToInt(y * scaleY),
            juce::roundToInt(w * scaleX), juce::roundToInt(h * scaleY)};
}

juce::ImageButton *ObxfAudioProcessorEditor::addMenuButton(const int x, const int y, const int d,
                                                           const juce::String &imgName)
{

    auto *imageButton = new ImageMenu(imgName, &processor);
    imageButtons.add(imageButton);
    imageButton->setBounds(x, y, d, d);

    imageButton->onClick = [this]() {
        const juce::ImageButton *image_button = this->imageButtons[0];
        auto x = image_button->getScreenX();
        auto y = image_button->getScreenY();
        auto dx = image_button->getWidth();
        auto pos = juce::Point<int>(x, y + dx);
        resultFromMenu(pos);
    };
    addAndMakeVisible(imageButton);
    return imageButton;
}

MidiKeyboard *ObxfAudioProcessorEditor::addMidiKeyboard(const int x, const int y, const int w,
                                                        const int h)
{
    if (midiKeyboard == nullptr)
    {
        midiKeyboard = std::make_unique<MidiKeyboard>(
            processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard,
            &processor);
    }

    midiKeyboard->setBounds(transformBounds(x, y, w, h));
    midiKeyboard->setScrollButtonsVisible(false);
    midiKeyboard->setMidiChannel(1);
    addAndMakeVisible(*midiKeyboard);
    return midiKeyboard.get();
}

void ObxfAudioProcessorEditor::rebuildComponents(ObxfAudioProcessor &ownerFilter)
{
    skinFolder = utils.getCurrentSkinFolder();

    ownerFilter.removeChangeListener(this);

    setSize(1440, 450);

    ownerFilter.addChangeListener(this);
    repaint();
}

void ObxfAudioProcessorEditor::createMenu()
{
    juce::MemoryBlock memoryBlock;
    memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());
    bool enablePasteOption = utils.isMemoryBlockAPatch(memoryBlock);
    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    skins = utils.getSkinFiles();
    banks = utils.getBankFiles();
    {
        juce::PopupMenu fileMenu;

        fileMenu.addItem(static_cast<int>(MenuAction::InitPatch), "Initialize Patch", true, false);
        fileMenu.addItem(static_cast<int>(MenuAction::NewPatch), "New Patch...",
                         true, // enableNewPresetOption,
                         false);
        fileMenu.addItem(static_cast<int>(MenuAction::RenamePatch), "Rename Patch...", true, false);
        fileMenu.addItem(static_cast<int>(MenuAction::SavePatch), "Save Patch...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::ImportPatch), "Import Patch...", true, false);
        fileMenu.addItem(static_cast<int>(MenuAction::ImportBank), "Import Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::ExportPatch), "Export Patch...", true, false);
        fileMenu.addItem(static_cast<int>(MenuAction::ExportBank), "Export Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::CopyPatch), "Copy Patch", true, false);
        fileMenu.addItem(static_cast<int>(MenuAction::PastePatch), "Paste Patch", enablePasteOption,
                         false);

        menu->addSubMenu("File", fileMenu);
    }

    {
        const uint8_t NUM_COLUMNS = 4;

        juce::PopupMenu patchMenu;

        for (int i = 0; i < processor.getNumPrograms(); ++i)
        {
            if (i > 0 && i % (processor.getNumPrograms() / NUM_COLUMNS) == 0)
            {
                patchMenu.addColumnBreak();
            }

            patchMenu.addItem(i + presetStart + 1,
                              juce::String{i + 1} + ": " + processor.getProgramName(i), true,
                              i == processor.getCurrentProgram());
        }

        menu->addSubMenu("Patches", patchMenu);
    }

    {
        juce::PopupMenu bankMenu;
        const juce::String currentBank = utils.getCurrentBankFile().getFileName();

        for (int i = 0; i < banks.size(); ++i)
        {
            const juce::File bank = banks.getUnchecked(i);
            bankMenu.addItem(i + bankStart + 1, bank.getFileNameWithoutExtension(), true,
                             bank.getFileName() == currentBank);
        }

        menu->addSubMenu("Banks", bankMenu);
    }

    menuMidiNum = presetStart + 2000;

    createMidi(menuMidiNum, midiMenu);

    menu->addSubMenu("MIDI Mappings", midiMenu);

    {
        juce::PopupMenu skinMenu;
        for (int i = 0; i < skins.size(); ++i)
        {
            const juce::File skin = skins.getUnchecked(i);
            skinMenu.addItem(i + skinStart + 1, skin.getFileName(), true,
                             skin.getFileName() == skinFolder.getFileName());
        }

        menu->addSubMenu("Themes", skinMenu);
    }

    {
        juce::PopupMenu sizeMenu;

        sizeMenu.addItem(sizeStart + 1, "75%", true, false);
        sizeMenu.addItem(sizeStart + 2, "100%", true, false);
        sizeMenu.addItem(sizeStart + 3, "125%", true, false);
        sizeMenu.addItem(sizeStart + 4, "150%", true, false);
        sizeMenu.addItem(sizeStart + 5, "175%", true, false);
        sizeMenu.addItem(sizeStart + 6, "200%", true, false);

        menu->addSubMenu("Zoom", sizeMenu);
    }

    menu->addSeparator();

    menu->addItem("About OB-Xf", [w = SafePointer(this)]() {
        if (!w)
            return;
        w->aboutScreen->showOver(w.getComponent());
    });

    popupMenus.add(menu);
}

void ObxfAudioProcessorEditor::createMidi(int menuNo, juce::PopupMenu &menuMidi)
{
    const juce::File midi_dir = utils.getMidiFolder();
    if (const juce::File default_file = midi_dir.getChildFile("Default.xml"); default_file.exists())
    {
        if (processor.getCurrentMidiPath() != default_file.getFullPathName())
        {
            menuMidi.addItem(menuNo++, default_file.getFileNameWithoutExtension(), true, false);
        }
        else
        {
            menuMidi.addItem(menuNo++, default_file.getFileNameWithoutExtension(), true, true);
        }
        midiFiles.add(default_file.getFullPathName());
    }

    if (const juce::File custom_file = midi_dir.getChildFile("Custom.xml"); custom_file.exists())
    {
        if (processor.getCurrentMidiPath() != custom_file.getFullPathName())
        {
            menuMidi.addItem(menuNo++, custom_file.getFileNameWithoutExtension(), true, false);
        }
        else
        {
            menuMidi.addItem(menuNo++, custom_file.getFileNameWithoutExtension(), true, true);
        }
        midiFiles.add(custom_file.getFullPathName());
    }

    juce::StringArray list;
    for (const auto &entry : juce::RangedDirectoryIterator(midi_dir, true, "*.xml"))
    {
        list.add(entry.getFile().getFullPathName());
    }

    list.sort(true);

    for (const auto &i : list)
    {
        if (juce::File f(i); f.getFileNameWithoutExtension() != "Default" &&
                             f.getFileNameWithoutExtension() != "Custom" &&
                             f.getFileNameWithoutExtension() != "Config")
        {
            if (processor.getCurrentMidiPath() != f.getFullPathName())
            {
                menuMidi.addItem(menuNo++, f.getFileNameWithoutExtension(), true, false);
            }
            else
            {
                menuMidi.addItem(menuNo++, f.getFileNameWithoutExtension(), true, true);
            }
            midiFiles.add(f.getFullPathName());
        }
    }
}

void ObxfAudioProcessorEditor::resultFromMenu(const juce::Point<int> pos)
{
    createMenu();

    popupMenus[0]->showMenuAsync(
        juce::PopupMenu::Options().withTargetScreenArea(
            juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1)),
        [this](int result) {
            if (result >= (skinStart + 1) && result <= (skinStart + skins.size()))
            {
                result -= 1;
                result -= skinStart;

                const juce::File newSkinFolder = skins.getUnchecked(result);
                utils.setCurrentSkinFolder(newSkinFolder.getFileName());

                clean();
                loadSkin(processor);
            }
            else if (result >= (bankStart + 1) && result <= (bankStart + banks.size()))
            {
                result -= 1;
                result -= bankStart;

                const juce::File bankFile = banks.getUnchecked(result);
                utils.loadFromFXBFile(bankFile);
            }
            else if (result >= (presetStart + 1) &&
                     result <= (presetStart + processor.getNumPrograms()))
            {
                result -= 1;
                result -= presetStart;
                processor.setCurrentProgram(result);
            }
            else if (result >= (sizeStart + 1) && result <= (sizeStart + 6))
            {
                const int initialWidth = backgroundImage.getWidth();
                const int initialHeight = backgroundImage.getHeight();

                constexpr float scaleFactors[] = {0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f};

                if (const int index = result - sizeStart - 1; index >= 0 && index < 6)
                {
                    const float scaleFactor = scaleFactors[index];

                    const int newWidth =
                        juce::roundToInt(static_cast<float>(initialWidth) * scaleFactor);
                    const int newHeight =
                        juce::roundToInt(static_cast<float>(initialHeight) * scaleFactor);

                    setSize(newWidth, newHeight);
                    resized();
                }
            }
            else if (result < presetStart)
            {
                MenuActionCallback(result);
            }
            else if (result >= menuMidiNum)
            {
                if (const auto selected_idx = result - menuMidiNum; selected_idx < midiFiles.size())
                {
                    if (juce::File f(midiFiles[selected_idx]); f.exists())
                    {
                        processor.getCurrentMidiPath() = midiFiles[selected_idx];
                        processor.bindings.loadFile(f);
                        processor.updateMidiConfig();
                    }
                }
            }
        });
}

void ObxfAudioProcessorEditor::updatePresetBar(const bool resize)
{
    if (resize)
    {
        setSize(getWidth(), getHeight() + presetBar->getHeight());
        resized();
        scaleFactorChanged();
        repaint();

        if (const auto parent = getParentComponent())
        {
            parent->resized();
            parent->repaint();
        }
    }

    presetBar->setVisible(true);
    presetBar->update();
    presetBar->setBounds((getWidth() - presetBar->getWidth()) / 2,
                         getHeight() - presetBar->getHeight(), presetBar->getWidth(),
                         presetBar->getHeight());
}

void ObxfAudioProcessorEditor::MenuActionCallback(int action)
{

    if (action == MenuAction::ImportBank)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Import Bank", juce::File(), "*.fxb", true);
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                const juce::File result = chooser.getResult();
                if (result != juce::File())
                {
                    const auto name = result.getFileName().replace("%20", " ");
                    const auto file = utils.getBanksFolder().getChildFile(name);

                    if (result == file || result.copyFileTo(file))
                    {
                        utils.loadFromFXBFile(file);
                        utils.scanAndUpdateBanks();
                    }
                }
            });
    };

    if (action == MenuAction::ExportBank)
    {
        const auto file = utils.getDocumentFolder().getChildFile("Banks");
        fileChooser = std::make_unique<juce::FileChooser>("Export Bank", file, "*.fxb", true);
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                     juce::FileBrowserComponent::canSelectFiles |
                                     juce::FileBrowserComponent::warnAboutOverwriting,
                                 [this](const juce::FileChooser &chooser) {
                                     const juce::File result = chooser.getResult();
                                     if (result != juce::File())
                                     {
                                         juce::String temp = result.getFullPathName();
                                         if (!temp.endsWith(".fxb"))
                                         {
                                             temp += ".fxb";
                                         }
                                         utils.saveBank(juce::File(temp));
                                     }
                                 });
    }

    if (action == MenuAction::DeleteBank)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle("Delete Bank")
                .withMessage("Delete current bank: " + utils.getCurrentBank() + "?")
                .withButton("OK")
                .withButton("Cancel"),
            [this](const int result) {
                if (result == 1)
                {
                    utils.deleteBank();
                }
            });
    }

    if (action == MenuAction::SavePatch)
    {
        if (const auto presetName = utils.getCurrentProgram(); presetName.isEmpty())
        {
            utils.saveBank();
            return;
        }
        utils.savePatch();
        utils.saveBank();
    }

    if (action == MenuAction::NewPatch)
    {
        setPresetNameWindow = std::make_unique<SetPresetNameWindow>();
        addAndMakeVisible(setPresetNameWindow.get());
        resized();

        auto callback = [this](const int i, const juce::String &name) {
            if (i)
            {
                if (name.isNotEmpty())
                {
                    utils.newPatch(name);
                }
            }
            setPresetNameWindow.reset();
        };

        setPresetNameWindow->callback = callback;
        setPresetNameWindow->grabTextEditorFocus();

        return;
    }

    if (action == MenuAction::RenamePatch)
    {
        setPresetNameWindow = std::make_unique<SetPresetNameWindow>();
        setPresetNameWindow->setText(processor.getProgramName(processor.getCurrentProgram()));
        addAndMakeVisible(setPresetNameWindow.get());
        resized();

        auto callback = [this](const int i, const juce::String &name) {
            if (i)
            {
                if (name.isNotEmpty())
                {
                    utils.changePatchName(name);
                }
            }
            setPresetNameWindow.reset();
        };

        setPresetNameWindow->callback = callback;
        setPresetNameWindow->grabTextEditorFocus();

        return;
    }

    if (action == MenuAction::InitPatch)
    {
        utils.initializePatch();
        processor.setCurrentProgram(processor.getCurrentProgram(), true);

        return;
    }

    if (action == MenuAction::ImportPatch)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Import Preset", juce::File(), "*.fxp", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    utils.loadPatch(result);
                }
            });
    }

    if (action == MenuAction::ExportPatch)
    {
        const auto file = utils.getPresetsFolder();
        fileChooser = std::make_unique<juce::FileChooser>("Export Preset", file, "*.fxp", true);
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                     juce::FileBrowserComponent::canSelectFiles |
                                     juce::FileBrowserComponent::warnAboutOverwriting,
                                 [this](const juce::FileChooser &chooser) {
                                     const juce::File result = chooser.getResult();
                                     if (result != juce::File())
                                     {
                                         juce::String temp = result.getFullPathName();
                                         if (!temp.endsWith(".fxp"))
                                         {
                                             temp += ".fxp";
                                         }
                                         utils.savePatch(juce::File(temp));
                                     }
                                 });
    }

    // Copy to clipboard
    if (action == MenuAction::CopyPatch)
    {
        juce::MemoryBlock serializedData;
        utils.serializePatch(serializedData);
        juce::SystemClipboard::copyTextToClipboard(serializedData.toBase64Encoding());
    }

    // Paste from clipboard
    if (action == MenuAction::PastePatch)
    {
        juce::MemoryBlock memoryBlock;
        memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());
        processor.loadFromMemoryBlock(memoryBlock);
    }
}

void ObxfAudioProcessorEditor::nextProgram()
{
    int cur = processor.getCurrentProgram() + 1;
    if (cur == processor.getNumPrograms())
    {
        cur = 0;
    }
    processor.setCurrentProgram(cur, false);

    needNotifytoHost = true;
    countTimer = 0;
}

void ObxfAudioProcessorEditor::prevProgram()
{
    int cur = processor.getCurrentProgram() - 1;
    if (cur < 0)
    {
        cur = processor.getNumPrograms() - 1;
    }
    processor.setCurrentProgram(cur, false);

    needNotifytoHost = true;
    countTimer = 0;
}

void ObxfAudioProcessorEditor::buttonClicked(juce::Button *b)
{

    if (const auto toggleButton = dynamic_cast<ToggleButton *>(b);
        toggleButton == midiUnlearnButton.get())
    {
        if (midiUnlearnButton->getToggleState())
        {
            countTimerForLed = 0;
            processor.getMidiMap().reset();
            processor.getMidiMap().set_default();
            processor.sendChangeMessage();
        }
    }
}

void ObxfAudioProcessorEditor::updateFromHost()
{
    for (const auto knobAttachment : knobAttachments)
    {
        knobAttachment->updateToSlider();
    }

    for (const auto buttonListAttachment : buttonListAttachments)
    {
        buttonListAttachment->updateToSlider();
    }

    repaint();
}

void ObxfAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster * /*source*/)
{
    updateFromHost();
}

void ObxfAudioProcessorEditor::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        resultFromMenu(e.getMouseDownScreenPosition());
    }
}

void ObxfAudioProcessorEditor::handleAsyncUpdate()
{
    scaleFactorChanged();
    repaint();
}

void ObxfAudioProcessorEditor::paint(juce::Graphics &g)
{

    const float newPhysicalPixelScaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (newPhysicalPixelScaleFactor != utils.getPixelScaleFactor())
    {
        utils.setPixelScaleFactor(newPhysicalPixelScaleFactor);
        scaleFactorChanged();
    }

    g.fillAll(juce::Colours::black.brighter(0.1f));

    // background gui
    g.drawImage(backgroundImage, 0, 0, getWidth(), getHeight() - presetBar->getHeight(), 0, 0,
                backgroundImage.getWidth(), backgroundImage.getHeight());
}

bool ObxfAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    juce::StringArray extensions;
    extensions.add(".fxp");
    extensions.add(".fxb");

    if (files.size() == 1)
    {
        const auto file = juce::File(files[0]);
        const juce::String ext = file.getFileExtension().toLowerCase();
        return file.existsAsFile() && extensions.contains(ext);
    }
    for (const auto &q : files)
    {
        auto file = juce::File(q);

        if (juce::String ext = file.getFileExtension().toLowerCase();
            ext == ".fxb" || ext == ".fxp")
        {
            return true;
        }
    }
    return false;
}

void ObxfAudioProcessorEditor::filesDropped(const juce::StringArray &files, int /*x*/, int /*y*/)
{
    if (files.size() == 1)
    {
        const auto file = juce::File(files[0]);

        if (const juce::String ext = file.getFileExtension().toLowerCase(); ext == ".fxp")
        {
            utils.loadPatch(file);
        }
        else if (ext == ".fxb")
        {
            const auto name = file.getFileName().replace("%20", " ");

            if (const auto result = utils.getBanksFolder().getChildFile(name);
                file.copyFileTo(result))
            {
                utils.loadFromFXBFile(result);
                utils.scanAndUpdateBanks();
            }
        }
    }
    else
    {
        int i = processor.getCurrentProgram();
        for (const auto &q : files)
        {
            auto file = juce::File(q);
            if (juce::String ext = file.getFileExtension().toLowerCase(); ext == ".fxp")
            {
                processor.setCurrentProgram(i++);
            }
            if (i >= processor.getNumPrograms())
            {
                i = 0;
            }
        }
        processor.sendChangeMessage();
    }
}