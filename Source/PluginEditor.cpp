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
#include "Gui/ImageButton.h"
#include "Utils.h"

//==============================================================================
ObxfAudioProcessorEditor::ObxfAudioProcessorEditor(ObxfAudioProcessor &p)
    : AudioProcessorEditor(&p), ScalableComponent(&p), processor(p), utils(p.getUtils()),
      paramManager(p.getParamManager()), skinFolder(utils.getSkinFolder()), progStart(3000),
      bankStart(2000), skinStart(1000), skins(utils.getSkinFiles()), banks(utils.getBankFiles())

{
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

    setResizable(true, true);

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
                    if (auto list =
                            addList(x, y, w, h, ownerFilter, LEGATOMODE, "Legato", "legato");
                        list != nullptr)
                    {
                        legatoSwitch = std::move(list);
                        mappingComps["legatoSwitch"] = legatoSwitch.get();
                    }
                }

                if (name == "voiceSwitch")
                {
                    if (auto list =
                            addList(x, y, w, h, ownerFilter, VOICE_COUNT, "Voices", "voices");
                        list != nullptr)
                    {
                        voiceSwitch = std::move(list);
                        mappingComps["voiceSwitch"] = voiceSwitch.get();
                    }
                }

                if (name == "menu")
                {
                    const auto imageButton = addMenuButton(x, y, d, "menu");
                    mappingComps["menu"] = imageButton;
                }

                if (name == "resonanceKnob")
                {
                    // resonance filter section
                    resonanceKnob = addKnob(x, y, d, ownerFilter, RESONANCE, 0.f);
                    mappingComps["resonanceKnob"] = resonanceKnob.get();
                }
                if (name == "cutoffKnob")
                {
                    // cutoff filter section
                    cutoffKnob = addKnob(x, y, d, ownerFilter, CUTOFF, 1.f);
                    mappingComps["cutoffKnob"] = cutoffKnob.get();
                }
                if (name == "filterEnvelopeAmtKnob")
                {
                    // env amt filter section
                    filterEnvelopeAmtKnob = addKnob(x, y, d, ownerFilter, ENVELOPE_AMT, 0.f);
                    mappingComps["filterEnvelopeAmtKnob"] = filterEnvelopeAmtKnob.get();
                }
                if (name == "multimodeKnob")
                {
                    // mix filter section
                    multimodeKnob = addKnob(x, y, d, ownerFilter, MULTIMODE, 0.f);
                    mappingComps["multimodeKnob"] = multimodeKnob.get();
                }

                if (name == "volumeKnob")
                {
                    // volume master section
                    volumeKnob = addKnob(x, y, d, ownerFilter, VOLUME, 0.5f);
                    mappingComps["volumeKnob"] = volumeKnob.get();
                }
                if (name == "portamentoKnob")
                {
                    // glide global section
                    portamentoKnob = addKnob(x, y, d, ownerFilter, PORTAMENTO, 0.f);
                    mappingComps["portamentoKnob"] = portamentoKnob.get();
                }
                if (name == "osc1PitchKnob")
                {
                    // osc1 oscilators section
                    osc1PitchKnob = addKnob(x, y, d, ownerFilter, OSC1P, 0.5f);
                    osc1PitchKnob->shiftDragCallback = [](const double value) {
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
                    // pulse width oscilators section
                    pulseWidthKnob = addKnob(x, y, d, ownerFilter, PW, 0.f);
                    mappingComps["pulseWidthKnob"] = pulseWidthKnob.get();
                }
                if (name == "osc2PitchKnob")
                {
                    // osc2 oscilators section
                    osc2PitchKnob = addKnob(x, y, d, ownerFilter, OSC2P, 0.5f);
                    osc2PitchKnob->shiftDragCallback = [](const double value) {
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
                    // osc1 mixer section
                    osc1MixKnob = addKnob(x, y, d, ownerFilter, OSC1MIX, 1.f);
                    mappingComps["osc1MixKnob"] = osc1MixKnob.get();
                }
                if (name == "osc2MixKnob")
                {
                    // osc2 mixer section
                    osc2MixKnob = addKnob(x, y, d, ownerFilter, OSC2MIX, 1.f);
                    mappingComps["osc2MixKnob"] = osc2MixKnob.get();
                }
                if (name == "noiseMixKnob")
                {
                    // noise mixer section
                    noiseMixKnob = addKnob(x, y, d, ownerFilter, NOISEMIX, 0.f);
                    mappingComps["noiseMixKnob"] = noiseMixKnob.get();
                }

                if (name == "xmodKnob")
                {
                    // cross mod oscilators section
                    xmodKnob = addKnob(x, y, d, ownerFilter, XMOD, 0.f);
                    mappingComps["xmodKnob"] = xmodKnob.get();
                }
                if (name == "osc2DetuneKnob")
                {
                    // detune oscilators section
                    osc2DetuneKnob = addKnob(x, y, d, ownerFilter, OSC2_DET, 0.f);
                    mappingComps["osc2DetuneKnob"] = osc2DetuneKnob.get();
                }

                if (name == "envPitchModKnob")
                {
                    // Pitch Env AMT
                    envPitchModKnob = addKnob(x, y, d, ownerFilter, ENVPITCH, 0.f);
                    mappingComps["envPitchModKnob"] = envPitchModKnob.get();
                }
                if (name == "brightnessKnob")
                {
                    // Bright AMT
                    brightnessKnob = addKnob(x, y, d, ownerFilter, BRIGHTNESS, 1.f);
                    mappingComps["brightnessKnob"] = brightnessKnob.get();
                }

                if (name == "attackKnob")
                {
                    // Attack Amplifier Envelope Section
                    attackKnob = addKnob(x, y, d, ownerFilter, LATK, 0.f);
                    mappingComps["attackKnob"] = attackKnob.get();
                }
                if (name == "decayKnob") // Decay Amplifier Envelope Section
                {
                    decayKnob = addKnob(x, y, d, ownerFilter, LDEC, 0.f);
                    mappingComps["decayKnob"] = decayKnob.get();
                }
                if (name == "sustainKnob")
                {
                    // Sustain Amplifier Envelope Section
                    sustainKnob = addKnob(x, y, d, ownerFilter, LSUS, 1.f);
                    mappingComps["sustainKnob"] = sustainKnob.get();
                }
                if (name == "releaseKnob")
                {
                    // Release Amplifier Envelope Section
                    releaseKnob = addKnob(x, y, d, ownerFilter, LREL, 0.f);
                    mappingComps["releaseKnob"] = releaseKnob.get();
                }

                if (name == "fattackKnob")
                {
                    // Attack Filter Envelope Section
                    fattackKnob = addKnob(x, y, d, ownerFilter, FATK, 0.f);
                    mappingComps["fattackKnob"] = fattackKnob.get();
                }
                if (name == "fdecayKnob")
                {
                    // Decay Filter Envelope Section
                    fdecayKnob = addKnob(x, y, d, ownerFilter, FDEC, 0.f);
                    mappingComps["fdecayKnob"] = fdecayKnob.get();
                }
                if (name == "fsustainKnob")
                {
                    // Sustain Filter Envelope Section
                    fsustainKnob = addKnob(x, y, d, ownerFilter, FSUS, 1.f);
                    mappingComps["fsustainKnob"] = fsustainKnob.get();
                }
                if (name == "freleaseKnob")
                {
                    // Release Filter Envelope Section
                    freleaseKnob = addKnob(x, y, d, ownerFilter, FREL, 0.f);
                    mappingComps["freleaseKnob"] = freleaseKnob.get();
                }

                if (name == "lfoFrequencyKnob")
                {
                    // LFO Rate MOD LFO section
                    lfoFrequencyKnob = addKnob(x, y, d, ownerFilter, LFOFREQ, 0.4925f); // 4 Hz
                    mappingComps["lfoFrequencyKnob"] = lfoFrequencyKnob.get();
                }
                if (name == "lfoAmt1Knob")
                {
                    // Freq AMT MOD LFO section
                    lfoAmt1Knob = addKnob(x, y, d, ownerFilter, LFO1AMT, 0.f);
                    mappingComps["lfoAmt1Knob"] = lfoAmt1Knob.get();
                }
                if (name == "lfoAmt2Knob")
                {
                    // PW AMT MOD LFO section
                    lfoAmt2Knob = addKnob(x, y, d, ownerFilter, LFO2AMT, 0.f);
                    mappingComps["lfoAmt2Knob"] = lfoAmt2Knob.get();
                }

                if (name == "lfoSinButton")
                {
                    // Sin Wave MOD LFO section
                    lfoSinButton = addButton(x, y, w, h, ownerFilter, LFOSINWAVE, "Sin");
                    mappingComps["lfoSinButton"] = lfoSinButton.get();
                }
                if (name == "lfoSquareButton")
                {
                    // Square Wave MOD LFO section
                    lfoSquareButton = addButton(x, y, w, h, ownerFilter, LFOSQUAREWAVE, "SQ");
                    mappingComps["lfoSquareButton"] = lfoSquareButton.get();
                }
                if (name == "lfoSHButton")
                {
                    // Sample & Hold MOD LFO section
                    lfoSHButton = addButton(x, y, w, h, ownerFilter, LFOSHWAVE, "S&H");
                    mappingComps["lfoSHButton"] = lfoSHButton.get();
                }

                if (name == "lfoOsc1Button")
                {
                    // Osc1 MOD LFO section
                    lfoOsc1Button = addButton(x, y, w, h, ownerFilter, LFOOSC1, "Osc1");
                    mappingComps["lfoOsc1Button"] = lfoOsc1Button.get();
                }
                if (name == "lfoOsc2Button")
                {
                    // Osc2 MOD LFO section
                    lfoOsc2Button = addButton(x, y, w, h, ownerFilter, LFOOSC2, "Osc2");
                    mappingComps["lfoOsc2Button"] = lfoOsc2Button.get();
                }
                if (name == "lfoFilterButton")
                {
                    // Filter MOD LFO section
                    lfoFilterButton = addButton(x, y, w, h, ownerFilter, LFOFILTER, "Filt");
                    mappingComps["lfoFilterButton"] = lfoFilterButton.get();
                }

                if (name == "lfoPwm1Button")
                {
                    // OSC1 PWM MOD LFO section
                    lfoPwm1Button = addButton(x, y, w, h, ownerFilter, LFOPW1, "Osc1");
                    mappingComps["lfoPwm1Button"] = lfoPwm1Button.get();
                }
                if (name == "lfoPwm2Button")
                {
                    // OSC2 PWM MOD LFO section
                    lfoPwm2Button = addButton(x, y, w, h, ownerFilter, LFOPW2, "Osc2");
                    mappingComps["lfoPwm2Button"] = lfoPwm2Button.get();
                }

                if (name == "hardSyncButton")
                {
                    // SYNC OSCILLATORS SECTION
                    hardSyncButton = addButton(x, y, w, h, ownerFilter, OSC2HS, "Sync");
                    mappingComps["hardSyncButton"] = hardSyncButton.get();
                }
                if (name == "osc1SawButton")
                {
                    // SAW WAVE OSCILLATORS SECTION
                    osc1SawButton = addButton(x, y, w, h, ownerFilter, OSC1Saw, "S");
                    mappingComps["osc1SawButton"] = osc1SawButton.get();
                }
                if (name == "osc2SawButton")
                {
                    // SAW WAVE OSCILLATORS SECTION 2
                    osc2SawButton = addButton(x, y, w, h, ownerFilter, OSC2Saw, "S");
                    mappingComps["osc2SawButton"] = osc2SawButton.get();
                }

                if (name == "osc1PulButton")
                {
                    // PULSE WAVE OSCILLATORS SECTION
                    osc1PulButton = addButton(x, y, w, h, ownerFilter, OSC1Pul, "P");
                    mappingComps["osc1PulButton"] = osc1PulButton.get();
                }
                if (name == "osc2PulButton")
                {
                    // PULSE WAVE OSCILLATORS SECTION 2
                    osc2PulButton = addButton(x, y, w, h, ownerFilter, OSC2Pul, "P");
                    mappingComps["osc2PulButton"] = osc2PulButton.get();
                }

                if (name == "pitchQuantButton")
                {
                    // STEP OSCILLATORS SECTION
                    pitchQuantButton = addButton(x, y, w, h, ownerFilter, OSCQuantize, "Step");
                    pitchQuantButton->onStateChange = [&] {
                        const auto isButtonOn = pitchQuantButton->getToggleState();
                        const auto configureOscKnob = [&](const juce::String &parameter) {
                            if (auto *knob = dynamic_cast<Knob *>(mappingComps[parameter]))
                            {
                                if (isButtonOn)
                                    knob->alternativeValueMapCallback = [](double value) {
                                        const auto semitoneValue =
                                            (int)juce::jmap(value, -24.0, 24.0);
                                        return juce::jmap((double)semitoneValue, -24.0, 24.0, 0.0,
                                                          1.0);
                                    };
                                else
                                    knob->alternativeValueMapCallback = nullptr;
                            }
                        };
                        configureOscKnob("osc1PitchKnob");
                        configureOscKnob("osc2PitchKnob");
                    };
                    mappingComps["pitchQuantButton"] = pitchQuantButton.get();
                }

                if (name == "filterBPBlendButton")
                {
                    // BP FILTER SECTION
                    filterBPBlendButton = addButton(x, y, w, h, ownerFilter, BANDPASS, "Bp");
                    mappingComps["filterBPBlendButton"] = filterBPBlendButton.get();
                }
                if (name == "fourPoleButton")
                {
                    // LP24 FILTER SECTION
                    fourPoleButton = addButton(x, y, w, h, ownerFilter, FOURPOLE, "24");
                    mappingComps["fourPoleButton"] = fourPoleButton.get();
                }
                if (name == "filterHQButton")
                {
                    filterHQButton = addButton(x, y, w, h, ownerFilter, FILTER_WARM, "HQ");
                    mappingComps["filterHQButton"] = filterHQButton.get();
                }

                if (name == "filterKeyFollowKnob")
                {
                    filterKeyFollowKnob = addKnob(x, y, d, ownerFilter, FLT_KF, 0.f);
                    mappingComps["filterKeyFollowKnob"] = filterKeyFollowKnob.get();
                }

                if (name == "unisonButton")
                {
                    // UNI GLOBAL SECTION
                    unisonButton = addButton(x, y, w, h, ownerFilter, UNISON, "Uni");
                    mappingComps["unisonButton"] = unisonButton.get();
                }

                if (name == "tuneKnob")
                {
                    // FINE MASTER SECTION
                    tuneKnob = addKnob(x, y, d, ownerFilter, TUNE, 0.5f);
                    mappingComps["tuneKnob"] = tuneKnob.get();
                }
                if (name == "transposeKnob")
                {
                    // COARSE MASTER SECTION
                    transposeKnob = addKnob(x, y, d, ownerFilter, OCTAVE, 0.5f);
                    mappingComps["transposeKnob"] = transposeKnob.get();
                }

                if (name == "voiceDetuneKnob")
                {
                    // SPREAD MASTER SECTION
                    voiceDetuneKnob = addKnob(x, y, d, ownerFilter, UDET, 0.25f);
                    mappingComps["voiceDetuneKnob"] = voiceDetuneKnob.get();
                }

                if (name == "bendLfoRateKnob")
                {
                    // VIBRATO RATE CONTROL SECTION
                    bendLfoRateKnob = addKnob(x, y, d, ownerFilter, BENDLFORATE, 0.4375f); // 4 Hz
                    mappingComps["bendLfoRateKnob"] = bendLfoRateKnob.get();
                }
                if (name == "veloFltEnvKnob")
                {
                    // FLT ENV VELOCITY CONTROL SECTION
                    veloFltEnvKnob = addKnob(x, y, d, ownerFilter, VFLTENV, 0.f);
                    mappingComps["veloFltEnvKnob"] = veloFltEnvKnob.get();
                }
                if (name == "veloAmpEnvKnob")
                {
                    // AMP ENV VELOCITY CONTROL SECTION
                    veloAmpEnvKnob = addKnob(x, y, d, ownerFilter, VAMPENV, 0.f);
                    mappingComps["veloAmpEnvKnob"] = veloAmpEnvKnob.get();
                }
                if (name == "midiLearnButton")
                {
                    // LEARN GLOBAL SECTION
                    midiLearnButton = addButton(x, y, w, h, ownerFilter, MIDILEARN, "LEA");
                    mappingComps["midiLearnButton"] = midiLearnButton.get();
                }
                if (name == "midiUnlearnButton")
                {
                    // CLEAR GLOBAL SECTION
                    midiUnlearnButton = addButton(x, y, w, h, ownerFilter, UNLEARN, "UNL");
                    mappingComps["midiUnlearnButton"] = midiUnlearnButton.get();
                }

                if (name == "pan1Knob")
                {
                    pan1Knob = addKnob(x, y, d, ownerFilter, PAN1, 0.5f);
                    pan1Knob->resetOnShiftClick(true, Action::panReset);
                    pan1Knob->addActionListener(this);
                    mappingComps["pan1Knob"] = pan1Knob.get();
                }
                if (name == "pan2Knob")
                {
                    pan2Knob = addKnob(x, y, d, ownerFilter, PAN2, 0.5f);
                    pan2Knob->resetOnShiftClick(true, Action::panReset);
                    pan2Knob->addActionListener(this);
                    mappingComps["pan2Knob"] = pan2Knob.get();
                }
                if (name == "pan3Knob")
                {
                    pan3Knob = addKnob(x, y, d, ownerFilter, PAN3, 0.5f);
                    pan3Knob->resetOnShiftClick(true, Action::panReset);
                    pan3Knob->addActionListener(this);
                    mappingComps["pan3Knob"] = pan3Knob.get();
                }
                if (name == "pan4Knob")
                {
                    pan4Knob = addKnob(x, y, d, ownerFilter, PAN4, 0.5f);
                    pan4Knob->resetOnShiftClick(true, Action::panReset);
                    pan4Knob->addActionListener(this);
                    mappingComps["pan4Knob"] = pan4Knob.get();
                }

                if (name == "pan5Knob")
                {
                    pan5Knob = addKnob(x, y, d, ownerFilter, PAN5, 0.5f);
                    pan5Knob->resetOnShiftClick(true, Action::panReset);
                    pan5Knob->addActionListener(this);
                    mappingComps["pan5Knob"] = pan5Knob.get();
                }
                if (name == "pan6Knob")
                {
                    pan6Knob = addKnob(x, y, d, ownerFilter, PAN6, 0.5f);
                    pan6Knob->resetOnShiftClick(true, Action::panReset);
                    pan6Knob->addActionListener(this);
                    mappingComps["pan6Knob"] = pan6Knob.get();
                }
                if (name == "pan7Knob")
                {
                    pan7Knob = addKnob(x, y, d, ownerFilter, PAN7, 0.5f);
                    pan7Knob->resetOnShiftClick(true, Action::panReset);
                    pan7Knob->addActionListener(this);
                    mappingComps["pan7Knob"] = pan7Knob.get();
                }
                if (name == "pan8Knob")
                {
                    pan8Knob = addKnob(x, y, d, ownerFilter, PAN8, 0.5f);
                    pan8Knob->resetOnShiftClick(true, Action::panReset);
                    pan8Knob->addActionListener(this);
                    mappingComps["pan8Knob"] = pan8Knob.get();
                }

                if (name == "bendOsc2OnlyButton")
                {
                    // BEND OSC2 CONTROL SECTION
                    bendOsc2OnlyButton = addButton(x, y, w, h, ownerFilter, BENDOSC2, "Osc2");
                    mappingComps["bendOsc2OnlyButton"] = bendOsc2OnlyButton.get();
                }
                if (name == "bendRangeButton")
                {
                    // BEND OCTAVE CONTROL SECTION
                    bendRangeButton = addButton(x, y, w, h, ownerFilter, BENDRANGE, "12");
                    mappingComps["bendRangeButton"] = bendRangeButton.get();
                }
                if (name == "asPlayedAllocButton")
                {
                    // VAM GLOBAL SECTION
                    asPlayedAllocButton =
                        addButton(x, y, w, h, ownerFilter, ASPLAYEDALLOCATION, "APA");
                    mappingComps["asPlayedAllocButton"] = asPlayedAllocButton.get();
                }

                if (name == "filterDetuneKnob")
                {
                    // FLT SLOP VOICE VARIATION SECTION
                    filterDetuneKnob = addKnob(x, y, d, ownerFilter, FILTERDER, 0.3f);
                    mappingComps["filterDetuneKnob"] = filterDetuneKnob.get();
                }
                if (name == "portamentoDetuneKnob")
                {
                    // GLD SLOP VOICE VARIATION SECTION
                    portamentoDetuneKnob = addKnob(x, y, d, ownerFilter, PORTADER, 0.3f);
                    mappingComps["portamentoDetuneKnob"] = portamentoDetuneKnob.get();
                }
                if (name == "envelopeDetuneKnob")
                {
                    // ENV SLOP VOICE VARIATION SECTION
                    envelopeDetuneKnob = addKnob(x, y, d, ownerFilter, ENVDER, 0.3f);
                    mappingComps["envelopeDetuneKnob"] = envelopeDetuneKnob.get();
                }
                if (name == "lfoSyncButton")
                {
                    lfoSyncButton = addButton(x, y, w, h, ownerFilter, LFO_SYNC, "Sync");
                    mappingComps["lfoSyncButton"] = lfoSyncButton.get();
                }
                if (name == "pwEnvKnob")
                {
                    pwEnvKnob = addKnob(x, y, d, ownerFilter, PW_ENV, 0.f);
                    mappingComps["pwEnvKnob"] = pwEnvKnob.get();
                }
                if (name == "pwEnvBothButton")
                {
                    pwEnvBothButton = addButton(x, y, w, h, ownerFilter, PW_ENV_BOTH, "PWEnv");
                    mappingComps["pwEnvBothButton"] = pwEnvBothButton.get();
                }
                if (name == "envPitchBothButton")
                {
                    envPitchBothButton = addButton(x, y, w, h, ownerFilter, ENV_PITCH_BOTH, "EnvP");
                    mappingComps["envPitchBothButton"] = envPitchBothButton.get();
                }
                if (name == "fenvInvertButton")
                {
                    fenvInvertButton = addButton(x, y, w, h, ownerFilter, FENV_INVERT, "FEnv");
                    mappingComps["fenvInvertButton"] = fenvInvertButton.get();
                }
                if (name == "pwOffsetKnob")
                {
                    pwOffsetKnob = addKnob(x, y, d, ownerFilter, PW_OSC2_OFS, 0.f);
                    mappingComps["pwOffsetKnob"] = pwOffsetKnob.get();
                }
                if (name == "selfOscPushButton")
                {
                    selfOscPushButton = addButton(x, y, w, h, ownerFilter, SELF_OSC_PUSH, "SOP");
                    mappingComps["selfOscPushButton"] = selfOscPushButton.get();
                }
            }
        }

        presetBar = std::make_unique<PresetBar>(*this);
        addAndMakeVisible(*presetBar);
        presetBar->leftClicked = [this](juce::Point<int> &pos) {
            juce::PopupMenu menu;
            for (int i = 0; i < processor.getNumPrograms(); ++i)
            {
                menu.addItem(i + progStart + 1, processor.getProgramName(i), true,
                             i == processor.getCurrentProgram());
            }

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                                   juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1)),
                               [this](int result) {
                                   if (result >= (progStart + 1) &&
                                       result <= (progStart + processor.getNumPrograms()))
                                   {
                                       result -= 1;
                                       result -= progStart;
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
                                  const juce::String & /*name*/, const juce::String &nameImg)
{
#if JUCE_WINDOWS || JUCE_LINUX
    auto *bl = new ButtonList((nameImg), h, &processor);
#else
    auto *bl = new ButtonList(nameImg, h, &processor);
#endif

    buttonListAttachments.add(new ButtonList::ButtonListAttachment(
        filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *bl));

    bl->setBounds(x, y, w, h);
    addAndMakeVisible(bl);

    return std::unique_ptr<ButtonList>(bl);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(const int x, const int y, const int d,
                                                        ObxfAudioProcessor &filter,
                                                        const int parameter, const float defval,
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

    if (parameter != UNLEARN)
    {
        toggleAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(
            filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *button));
    }
    else
    {
        button->addListener(this);
    }
    button->setBounds(x, y, w, h);
    button->setButtonText(name);
    button->setToggleState(filter.getValueTreeState()
                               .getParameter(paramManager.getEngineParameterId(parameter))
                               ->getValue(),
                           juce::dontSendNotification);

    addAndMakeVisible(button);

    return std::unique_ptr<ToggleButton>(button);
}

void ObxfAudioProcessorEditor::actionListenerCallback(const juce::String &message)
{
    if (message.equalsIgnoreCase(Action::panReset))
    {
        const juce::StringArray parameters{"pan1Knob", "pan2Knob", "pan3Knob", "pan4Knob",
                                           "pan5Knob", "pan6Knob", "pan7Knob", "pan8Knob"};
        for (const auto &parameter : parameters)
        {
            if (auto *knob = dynamic_cast<Knob *>(mappingComps[parameter]))
            {
                knob->setValue(knob->getDoubleClickReturnValue());
            }
        }
    }
}

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
    bool enablePasteOption = utils.isMemoryBlockAPreset(memoryBlock);
    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    skins = utils.getSkinFiles();
    banks = utils.getBankFiles();
    {
        juce::PopupMenu fileMenu;

        fileMenu.addItem(static_cast<int>(MenuAction::ImportPreset), "Import Preset...", true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::ImportBank), "Import Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::ExportPreset), "Export Preset...", true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::ExportBank), "Export Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::NewPreset), "New Preset...",
                         true, // enableNewPresetOption,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::RenamePreset), "Rename Preset...", true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::SavePreset), "Save Preset...", true, false);

        fileMenu.addItem(static_cast<int>(MenuAction::DeletePreset), "Delete Preset...", true,
                         false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::CopyPreset), "Copy Preset...", true, false);

        fileMenu.addItem(static_cast<int>(MenuAction::PastePreset), "Paste Preset...",
                         enablePasteOption, false);

        menu->addSubMenu("File", fileMenu);
    }
    {
        juce::PopupMenu progMenu;
        for (int i = 0; i < processor.getNumPrograms(); ++i)
        {
            progMenu.addItem(i + progStart + 1, processor.getProgramName(i), true,
                             i == processor.getCurrentProgram());
        }

        menu->addSubMenu("Programs", progMenu);
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

    menuMidiNum = progStart + 2000;
    createMidi(menuMidiNum, midiMenu);
    menu->addSubMenu("MIDI", midiMenu);
    popupMenus.add(menu);

#ifdef LINUX
    menu->addItem(
        1, juce::String("Release ") + juce::String(JucePlugin_VersionString).dropLastCharacters(2),
        false);
#endif

#if defined(JUCE_MAC) || defined(WIN32)
    juce::PopupMenu helpMenu;
    juce::String version =
        juce::String("Release ") + juce::String(JucePlugin_VersionString).dropLastCharacters(2);
    helpMenu.addItem(menuScaleNum + 4, "Manual", true);
    helpMenu.addItem(menuScaleNum + 3, version, false);
    menu->addSubMenu("Help", helpMenu, true);
#endif
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
            else if (result >= (progStart + 1) &&
                     result <= (progStart + processor.getNumPrograms()))
            {
                result -= 1;
                result -= progStart;
                processor.setCurrentProgram(result);
            }
            else if (result < progStart)
            {
                MenuActionCallback(result);
            }
            else if (result >= menuScaleNum)
            {

                if (result == menuScaleNum + 4)
                {
                    const juce::File manualFile =
                        utils.getDocumentFolder().getChildFile("OB-Xd Manual.pdf");
                    utils.openInPdf(manualFile);
                }
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
            std::make_unique<juce::FileChooser>("Import Bank (*.fxb)", juce::File(), "*.fxb", true);
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
        fileChooser =
            std::make_unique<juce::FileChooser>("Export Bank (*.fxb)", file, "*.fxb", true);
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

    if (action == MenuAction::SavePreset)
    {
        if (const auto presetName = utils.getCurrentPreset(); presetName.isEmpty())
        {
            utils.saveBank();
            return;
        }
        utils.savePreset();
        utils.saveBank();
    }

    if (action == MenuAction::NewPreset)
    {
        setPresetNameWindow = std::make_unique<SetPresetNameWindow>();
        addAndMakeVisible(setPresetNameWindow.get());
        resized();

        auto callback = [this](const int i, const juce::String &name) {
            if (i)
            {
                if (name.isNotEmpty())
                {
                    utils.newPreset(name);
                }
            }
            setPresetNameWindow.reset();
        };

        setPresetNameWindow->callback = callback;
        setPresetNameWindow->grabTextEditorFocus();

        return;
    }

    if (action == MenuAction::RenamePreset)
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
                    utils.changePresetName(name);
                }
            }
            setPresetNameWindow.reset();
        };

        setPresetNameWindow->callback = callback;
        setPresetNameWindow->grabTextEditorFocus();

        return;
    }

    if (action == MenuAction::DeletePreset)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle("Delete Preset")
                .withMessage("Delete current preset " +
                             processor.getProgramName(processor.getCurrentProgram()) + "?")
                .withButton("OK")
                .withButton("Cancel"),
            [this](const int result) {
                if (result == 1)
                {
                    utils.deletePreset();
                }
            });
        return;
    }

    if (action == MenuAction::ImportPreset)
    {
        fileChooser = std::make_unique<juce::FileChooser>("Import Preset (*.fxp)", juce::File(),
                                                          "*.fxp", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    utils.loadPreset(result);
                }
            });
    }

    if (action == MenuAction::ExportPreset)
    {
        const auto file = utils.getPresetsFolder();
        fileChooser =
            std::make_unique<juce::FileChooser>("Export Preset (*.fxp)", file, "*.fxp", true);
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
                                         utils.savePreset(juce::File(temp));
                                     }
                                 });
    }

    // Copy to clipboard
    if (action == MenuAction::CopyPreset)
    {
        juce::MemoryBlock serializedData;
        utils.serializePreset(serializedData);
        juce::SystemClipboard::copyTextToClipboard(serializedData.toBase64Encoding());
    }

    // Paste from clipboard
    if (action == MenuAction::PastePreset)
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
            utils.loadPreset(file);
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

const juce::String ObxfAudioProcessorEditor::Action::panReset{"panReset"};
