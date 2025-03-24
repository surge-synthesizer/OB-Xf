#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Gui/ImageButton.h"
#include "Utils.h"


//==============================================================================
ObxdAudioProcessorEditor::ObxdAudioProcessorEditor(ObxdAudioProcessor &p)
    : AudioProcessorEditor(&p), ScalableComponent(&p), processor(p), utils(p.getUtils()),
      paramManager(p.getParamManager()),
      skinFolder(utils.getSkinFolder()),
      progStart(3000),
      bankStart(2000),
      skinStart(1000),
      skins(utils.getSkinFiles()),
      banks(utils.getBankFiles())

{
    commandManager.registerAllCommandsForTarget(this);
    commandManager.setFirstCommandTarget(this);
    commandManager.getKeyMappings()->resetToDefaultMappings();
    getTopLevelComponent()->addKeyListener(commandManager.getKeyMappings());

    if (juce::PluginHostType().isProTools())
    {
    }
    else { startTimer(100); }; // Fix ProTools file dialog focus issues

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


void ObxdAudioProcessorEditor::resized()
{
    if (setPresetNameWindow != nullptr)
    {
        if (const auto wrapper = dynamic_cast<ObxdAudioProcessorEditor *>(processor.
            getActiveEditor()))
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
                const bool tooltipEnabled = child->getBoolAttribute("tooltip", false);
                if (mappingComps[name] != nullptr)
                {
                    if (auto *knob = dynamic_cast<Knob *>(mappingComps[name]))
                    {
                        knob->setBounds(transformBounds(x, y, d, d));
                        if (const auto tooltipBehavior = utils.getTooltipBehavior();
                            tooltipBehavior == Tooltip::Disable ||
                            (tooltipBehavior == Tooltip::StandardDisplay && !tooltipEnabled))
                        {
                            knob->setPopupDisplayEnabled(false, false, nullptr);
                        }
                        else
                        {
                            knob->setPopupDisplayEnabled(true, true, knob->getParentComponent());
                        }
                    }
                    else if (dynamic_cast<ButtonList *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }

                    else if (dynamic_cast<TooglableButton *>(mappingComps[name]))
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
                }
            }
        }
    }
}

void ObxdAudioProcessorEditor::loadSkin(ObxdAudioProcessor &ownerFilter)
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
        setSize(1440, 450);
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

                if (name == "legatoSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, LEGATOMODE, "Legato", "legato")
                        ; list != nullptr)
                    {
                        legatoSwitch = std::move(list);
                        mappingComps["legatoSwitch"] = legatoSwitch.get();
                    }
                }

                if (name == "voiceSwitch")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, VOICE_COUNT, "Voices",
                                            "voices"); list != nullptr)
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
                    resonanceKnob = addKnob(x, y, d, ownerFilter, RESONANCE, "Resonance", 0);
                    mappingComps["resonanceKnob"] = resonanceKnob.get();
                }
                if (name == "cutoffKnob")
                {
                    // cutoff filter section
                    cutoffKnob = addKnob(x, y, d, ownerFilter, CUTOFF, "Cutoff", 0.4);
                    mappingComps["cutoffKnob"] = cutoffKnob.get();
                }
                if (name == "filterEnvelopeAmtKnob")
                {
                    // env amt filter section
                    filterEnvelopeAmtKnob = addKnob(x, y, d, ownerFilter, ENVELOPE_AMT, "Envelope",
                                                    0);
                    mappingComps["filterEnvelopeAmtKnob"] = filterEnvelopeAmtKnob.get();
                }
                if (name == "multimodeKnob")
                {
                    // mix filter section
                    multimodeKnob = addKnob(x, y, d, ownerFilter, MULTIMODE, "Multimode", 0.5);
                    mappingComps["multimodeKnob"] = multimodeKnob.get();
                }

                if (name == "volumeKnob")
                {
                    //volume master section
                    volumeKnob = addKnob(x, y, d, ownerFilter, VOLUME, "Volume", 0.4);
                    mappingComps["volumeKnob"] = volumeKnob.get();
                }
                if (name == "portamentoKnob")
                {
                    //glide global section
                    portamentoKnob = addKnob(x, y, d, ownerFilter, PORTAMENTO, "Portamento", 0);
                    mappingComps["portamentoKnob"] = portamentoKnob.get();
                }
                if (name == "osc1PitchKnob")
                {
                    //osc1 oscilators section
                    osc1PitchKnob = addKnob(x, y, d, ownerFilter, OSC1P, "Osc1Pitch", 0);
                    osc1PitchKnob->shiftDragCallback = [](const double value) {
                        if (value < 0.125)
                            return 0.0;
                        if (value < 0.375)
                            return 0.25;
                        if (value < 0.625)
                            return 0.5;
                        if (value < 0.875)
                            return 0.75;
                        return 1.0;
                    };
                    osc1PitchKnob->altDragCallback = [](const double value) {
                        const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
                        return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0,
                                          1.0);
                    };
                    mappingComps["osc1PitchKnob"] = osc1PitchKnob.get();
                }
                if (name == "pulseWidthKnob")
                {
                    //pulse width oscilators section
                    pulseWidthKnob = addKnob(x, y, d, ownerFilter, PW, "PW", 0);
                    mappingComps["pulseWidthKnob"] = pulseWidthKnob.get();
                }
                if (name == "osc2PitchKnob")
                {
                    //osc2 oscilators section
                    osc2PitchKnob = addKnob(x, y, d, ownerFilter, OSC2P, "Osc2Pitch", 0);
                    osc2PitchKnob->shiftDragCallback = [](const double value) {
                        if (value < 0.125)
                            return 0.0;
                        if (value < 0.375)
                            return 0.25;
                        if (value < 0.625)
                            return 0.5;
                        if (value < 0.875)
                            return 0.75;
                        return 1.0;
                    };
                    osc2PitchKnob->altDragCallback = [](const double value) {
                        const auto semitoneValue = (int)juce::jmap(value, -24.0, 24.0);
                        return juce::jmap((double)semitoneValue, -24.0, 24.0, 0.0, 1.0);
                    };
                    mappingComps["osc2PitchKnob"] = osc2PitchKnob.get();
                }
                //
                if (name == "osc1MixKnob")
                {
                    //osc1 mixer section
                    osc1MixKnob = addKnob(x, y, d, ownerFilter, OSC1MIX, "Osc1", 1);
                    mappingComps["osc1MixKnob"] = osc1MixKnob.get();
                }
                if (name == "osc2MixKnob")
                {
                    //osc2 mixer section
                    osc2MixKnob = addKnob(x, y, d, ownerFilter, OSC2MIX, "Osc2", 1);
                    mappingComps["osc2MixKnob"] = osc2MixKnob.get();
                }
                if (name == "noiseMixKnob")
                {
                    //noise mixer section
                    noiseMixKnob = addKnob(x, y, d, ownerFilter, NOISEMIX, "Noise", 0);
                    mappingComps["noiseMixKnob"] = noiseMixKnob.get();
                }

                if (name == "xmodKnob")
                {
                    //cross mod oscilators section
                    xmodKnob = addKnob(x, y, d, ownerFilter, XMOD, "Xmod", 0);
                    mappingComps["xmodKnob"] = xmodKnob.get();
                }
                if (name == "osc2DetuneKnob")
                {
                    //detune oscilators section
                    osc2DetuneKnob = addKnob(x, y, d, ownerFilter, OSC2_DET, "Detune", 0);
                    mappingComps["osc2DetuneKnob"] = osc2DetuneKnob.get();
                }

                if (name == "envPitchModKnob")
                {
                    //Pitch Env AMT
                    envPitchModKnob = addKnob(x, y, d, ownerFilter, ENVPITCH, "PEnv", 0);
                    mappingComps["envPitchModKnob"] = envPitchModKnob.get();
                }
                if (name == "brightnessKnob")
                {
                    // Bright AMT
                    brightnessKnob = addKnob(x, y, d, ownerFilter, BRIGHTNESS, "Bri", 1);
                    mappingComps["brightnessKnob"] = brightnessKnob.get();
                }

                if (name == "attackKnob")
                {
                    //Attack Amplifier Envelope Section
                    attackKnob = addKnob(x, y, d, ownerFilter, LATK, "Atk", 0);
                    mappingComps["attackKnob"] = attackKnob.get();
                }
                if (name == "decayKnob") //Decay Amplifier Envelope Section
                {
                    decayKnob = addKnob(x, y, d, ownerFilter, LDEC, "Dec", 0);
                    mappingComps["decayKnob"] = decayKnob.get();
                }
                if (name == "sustainKnob")
                {
                    // Sustain Amplifier Envelope Section
                    sustainKnob = addKnob(x, y, d, ownerFilter, LSUS, "Sus", 1);
                    mappingComps["sustainKnob"] = sustainKnob.get();
                }
                if (name == "releaseKnob")
                {
                    // Release Amplifier Envelope Section
                    releaseKnob = addKnob(x, y, d, ownerFilter, LREL, "Rel", 0);
                    mappingComps["releaseKnob"] = releaseKnob.get();
                }

                if (name == "fattackKnob")
                {
                    // Attack Filter Envelope Section
                    fattackKnob = addKnob(x, y, d, ownerFilter, FATK, "Atk", 0);
                    mappingComps["fattackKnob"] = fattackKnob.get();
                }
                if (name == "fdecayKnob")
                {
                    // Decay Filter Envelope Section
                    fdecayKnob = addKnob(x, y, d, ownerFilter, FDEC, "Dec", 0);
                    mappingComps["fdecayKnob"] = fdecayKnob.get();
                }
                if (name == "fsustainKnob")
                {
                    // Sustain Filter Envelope Section
                    fsustainKnob = addKnob(x, y, d, ownerFilter, FSUS, "Sus", 1);
                    mappingComps["fsustainKnob"] = fsustainKnob.get();
                }
                if (name == "freleaseKnob")
                {
                    // Release Filter Envelope Section
                    freleaseKnob = addKnob(x, y, d, ownerFilter, FREL, "Rel", 0);
                    mappingComps["freleaseKnob"] = freleaseKnob.get();
                }

                if (name == "lfoFrequencyKnob")
                {
                    // LFO Rate MOD LFO section
                    lfoFrequencyKnob = addKnob(x, y, d, ownerFilter, LFOFREQ, "Freq", 0);
                    mappingComps["lfoFrequencyKnob"] = lfoFrequencyKnob.get();
                }
                if (name == "lfoAmt1Knob")
                {
                    // Freq AMT MOD LFO section
                    lfoAmt1Knob = addKnob(x, y, d, ownerFilter, LFO1AMT, "Pitch", 0);
                    mappingComps["lfoAmt1Knob"] = lfoAmt1Knob.get();
                }
                if (name == "lfoAmt2Knob")
                {
                    // PW AMT MOD LFO section
                    lfoAmt2Knob = addKnob(x, y, d, ownerFilter, LFO2AMT, "PWM", 0);
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
                                        const auto semitoneValue = (int)juce::jmap(
                                            value, -24.0, 24.0);
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
                    //LP24 FILTER SECTION
                    fourPoleButton = addButton(x, y, w, h, ownerFilter, FOURPOLE, "24");
                    mappingComps["fourPoleButton"] = fourPoleButton.get();
                }
                if (name == "filterHQButton")
                {
                    filterHQButton = addButton(x, y, w, h, ownerFilter, FILTER_WARM, "HQ");
                    mappingComps["filterHQButton"] = filterHQButton.get();
                }

                if (name == "filterKeyFollowButton")
                {
                    filterKeyFollowButton = addButton(x, y, w, h, ownerFilter, FLT_KF, "Key");
                    mappingComps["filterKeyFollowButton"] = filterKeyFollowButton.get();
                }
                if (name == "unisonButton")
                {
                    // UNI GLOBAL SECTION
                    unisonButton = addButton(x, y, w, h, ownerFilter, UNISON, "Uni");
                    mappingComps["unisonButton"] = unisonButton.get();
                }

                if (name == "tuneKnob")
                {
                    //FINE MASTER SECTION
                    tuneKnob = addKnob(x, y, d, ownerFilter, TUNE, "Tune", 0.5);
                    mappingComps["tuneKnob"] = tuneKnob.get();
                }
                if (name == "transposeKnob")
                {
                    // COARSE MASTER SECTION
                    transposeKnob = addKnob(x, y, d, ownerFilter, OCTAVE, "Transpose", 0.5);
                    mappingComps["transposeKnob"] = transposeKnob.get();
                }

                if (name == "voiceDetuneKnob")
                {
                    // SPREAD MASTER SECTION
                    voiceDetuneKnob = addKnob(x, y, d, ownerFilter, UDET, "VoiceDet", 0);
                    mappingComps["voiceDetuneKnob"] = voiceDetuneKnob.get();
                }

                if (name == "bendLfoRateKnob")
                {
                    // VIBRATO RATE CONTROL SECTION
                    bendLfoRateKnob = addKnob(x, y, d, ownerFilter, BENDLFORATE, "ModRate", 0.4);
                    mappingComps["bendLfoRateKnob"] = bendLfoRateKnob.get();
                }
                if (name == "veloFltEnvKnob")
                {
                    // FLT ENV VELOCITY CONTROL SECTION
                    veloFltEnvKnob = addKnob(x, y, d, ownerFilter, VFLTENV, "VFE", 0);
                    mappingComps["veloFltEnvKnob"] = veloFltEnvKnob.get();
                }
                if (name == "veloAmpEnvKnob")
                {
                    // AMP ENV VELOCITY CONTROL SECTION
                    veloAmpEnvKnob = addKnob(x, y, d, ownerFilter, VAMPENV, "VAE", 0);
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
                    pan1Knob = addKnob(x, y, d, ownerFilter, PAN1, "1", 0.5);
                    pan1Knob->resetOnShiftClick(true, Action::panReset);
                    pan1Knob->addActionListener(this);
                    mappingComps["pan1Knob"] = pan1Knob.get();
                }
                if (name == "pan2Knob")
                {
                    pan2Knob = addKnob(x, y, d, ownerFilter, PAN2, "2", 0.5);
                    pan2Knob->resetOnShiftClick(true, Action::panReset);
                    pan2Knob->addActionListener(this);
                    mappingComps["pan2Knob"] = pan2Knob.get();
                }
                if (name == "pan3Knob")
                {
                    pan3Knob = addKnob(x, y, d, ownerFilter, PAN3, "3", 0.5);
                    pan3Knob->resetOnShiftClick(true, Action::panReset);
                    pan3Knob->addActionListener(this);
                    mappingComps["pan3Knob"] = pan3Knob.get();
                }
                if (name == "pan4Knob")
                {
                    pan4Knob = addKnob(x, y, d, ownerFilter, PAN4, "4", 0.5);
                    pan4Knob->resetOnShiftClick(true, Action::panReset);
                    pan4Knob->addActionListener(this);
                    mappingComps["pan4Knob"] = pan4Knob.get();
                }

                if (name == "pan5Knob")
                {
                    pan5Knob = addKnob(x, y, d, ownerFilter, PAN5, "5", 0.5);
                    pan5Knob->resetOnShiftClick(true, Action::panReset);
                    pan5Knob->addActionListener(this);
                    mappingComps["pan5Knob"] = pan5Knob.get();
                }
                if (name == "pan6Knob")
                {
                    pan6Knob = addKnob(x, y, d, ownerFilter, PAN6, "6", 0.5);
                    pan6Knob->resetOnShiftClick(true, Action::panReset);
                    pan6Knob->addActionListener(this);
                    mappingComps["pan6Knob"] = pan6Knob.get();
                }
                if (name == "pan7Knob")
                {
                    pan7Knob = addKnob(x, y, d, ownerFilter, PAN7, "7", 0.5);
                    pan7Knob->resetOnShiftClick(true, Action::panReset);
                    pan7Knob->addActionListener(this);
                    mappingComps["pan7Knob"] = pan7Knob.get();
                }
                if (name == "pan8Knob")
                {
                    pan8Knob = addKnob(x, y, d, ownerFilter, PAN8, "8", 0.5);
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
                    asPlayedAllocButton = addButton(x, y, w, h, ownerFilter, ASPLAYEDALLOCATION,
                                                    "APA");
                    mappingComps["asPlayedAllocButton"] = asPlayedAllocButton.get();
                }

                if (name == "filterDetuneKnob")
                {
                    // FLT SLOP VOICE VARIATION SECTION
                    filterDetuneKnob = addKnob(x, y, d, ownerFilter, FILTERDER, "Flt", 0.2);
                    mappingComps["filterDetuneKnob"] = filterDetuneKnob.get();
                }
                if (name == "portamentoDetuneKnob")
                {
                    // GLD SLOP VOICE VARIATION SECTION
                    portamentoDetuneKnob = addKnob(x, y, d, ownerFilter, PORTADER, "Port", 0.2);
                    mappingComps["portamentoDetuneKnob"] = portamentoDetuneKnob.get();
                }
                if (name == "envelopeDetuneKnob")
                {
                    // ENV SLOP VOICE VARIATION SECTION
                    envelopeDetuneKnob = addKnob(x, y, d, ownerFilter, ENVDER, "Env", 0.2);
                    mappingComps["envelopeDetuneKnob"] = envelopeDetuneKnob.get();
                }
            }
        }

        presetBar = std::make_unique<PresetBar>(*this);
        addAndMakeVisible(*presetBar);
        presetBar->setVisible(utils.getShowPresetBar());
        presetBar->leftClicked = [this](juce::Point<int> &pos) {
            juce::PopupMenu menu;
            for (int i = 0; i < processor.getNumPrograms(); ++i)
            {
                menu.addItem(i + progStart + 1,
                             processor.getProgramName(i),
                             true,
                             i == processor.getCurrentProgram());
            }

            if (int result = menu.showAt(juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1));
                result >= (progStart + 1) && result <= (progStart + processor.getNumPrograms()))
            {
                result -= 1;
                result -= progStart;
                processor.setCurrentProgram(result);
            }
        };
        resized();
    }

    // Prepare data
    if (voiceSwitch)
    {
        for (int i = 1; i <= 32; ++i)
        {
            voiceSwitch->addChoice(juce::String(i));
        }

        const auto voiceOption = ownerFilter.getValueTreeState().getParameter(
            paramManager.getEngineParameterId(VOICE_COUNT))->getValue();
        voiceSwitch->setValue(voiceOption, juce::dontSendNotification);
    }

    if (legatoSwitch)
    {
        legatoSwitch->addChoice("Keep All");
        legatoSwitch->addChoice("Keep Filter Envelope");
        legatoSwitch->addChoice("Keep Amplitude Envelope");
        legatoSwitch->addChoice("Retrig");
        const auto legatoOption = ownerFilter.getValueTreeState().getParameter(
            paramManager.getEngineParameterId(LEGATOMODE))->getValue();
        legatoSwitch->setValue(legatoOption, juce::dontSendNotification);
    }


    createMenu();

    ownerFilter.addChangeListener(this);

    scaleFactorChanged();
    repaint();
}

ObxdAudioProcessorEditor::~ObxdAudioProcessorEditor()
{
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    imageButtons.clear();
    popupMenus.clear();
    mappingComps.clear();

}

void ObxdAudioProcessorEditor::scaleFactorChanged()
{
    backgroundImage = getScaledImageFromCache("main");

    if (utils.getShowPresetBar() && presetBar != nullptr)
    {
        presetBar->setBounds((getWidth() - presetBar->getWidth()) / 2,
                             getHeight() - presetBar->getHeight(),
                             presetBar->getWidth(),
                             presetBar->getHeight());
    }

    resized();
}


void ObxdAudioProcessorEditor::placeLabel(const int x, const int y, const juce::String &text)
{
    auto *lab = new juce::Label();
    lab->setBounds(x, y, 110, 20);
    lab->setJustificationType(juce::Justification::centred);
    lab->setText(text, juce::dontSendNotification);
    lab->setInterceptsMouseClicks(false, true);
    addAndMakeVisible(lab);
}

std::unique_ptr<ButtonList> ObxdAudioProcessorEditor::addList(
    const int x, const int y, const int w, const int h,
    ObxdAudioProcessor &filter, const int parameter,
    const juce::String & /*name*/, const juce::String &nameImg)
{
#if JUCE_WINDOWS || JUCE_LINUX
    auto *bl = new ButtonList((nameImg), h, &processor);
#else
    auto *bl = new ButtonList(nameImg, h, &processor);
#endif

    buttonListAttachments.add(new ButtonList::ButtonListAttachment(filter.getValueTreeState(),
        paramManager.getEngineParameterId(parameter),
        *bl));

    bl->setBounds(x, y, w, h);
    addAndMakeVisible(bl);

    return std::unique_ptr<ButtonList>(bl);

}

std::unique_ptr<Knob> ObxdAudioProcessorEditor::addKnob(const int x, const int y, const int d,
                                                        ObxdAudioProcessor &filter,
                                                        const int parameter,
                                                        const juce::String & /*name*/,
                                                        const float defval)
{
    const auto knob = new Knob("knob", 48, &processor);

    knobAttachments.add(new Knob::KnobAttachment(filter.getValueTreeState(),
                                                 paramManager.getEngineParameterId(
                                                     parameter),
                                                 *knob));

    knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob->setTextBoxStyle(Knob::NoTextBox, true, 0, 0);
    knob->setRange(0, 1);
    knob->setBounds(x, y, d + (d / 6), d + (d / 6));
    knob->setTextBoxIsEditable(false);
    knob->setDoubleClickReturnValue(true, defval, juce::ModifierKeys::noModifiers);
    knob->setValue(
        filter.getValueTreeState().getParameter(
            paramManager.getEngineParameterId(parameter))->
        getValue());
    addAndMakeVisible(knob);

    return std::unique_ptr<Knob>(knob);
}


void ObxdAudioProcessorEditor::clean()
{
    this->removeAllChildren();
}

std::unique_ptr<TooglableButton> ObxdAudioProcessorEditor::addButton(
    const int x, const int y, const int w, const int h,
    ObxdAudioProcessor &filter, const int parameter,
    const juce::String &name)
{
    auto *button = new TooglableButton("button", &processor);

    if (parameter != UNLEARN)
    {
        toggleAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(
            filter.getValueTreeState(),
            paramManager.getEngineParameterId(parameter),
            *button));
    }
    else
    {
        button->addListener(this);
    }
    button->setBounds(x, y, w, h);
    button->setButtonText(name);
    button->setToggleState(
        filter.getValueTreeState().getParameter(
            paramManager.getEngineParameterId(parameter))->
        getValue(),
        juce::dontSendNotification);

    addAndMakeVisible(button);

    return std::unique_ptr<TooglableButton>(button);
}

void ObxdAudioProcessorEditor::actionListenerCallback(const juce::String &message)
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

juce::Rectangle<int> ObxdAudioProcessorEditor::transformBounds(int x, int y, int w, int h) const
{
    if (originalBounds.isEmpty())
        return {x, y, w, h};

    const int effectiveHeight = utils.getShowPresetBar() ? getHeight() - presetBar->getHeight() : getHeight();
    const float scaleX = getWidth() / static_cast<float>(originalBounds.getWidth());
    const float scaleY = effectiveHeight / static_cast<float>(originalBounds.getHeight());


    return {
        juce::roundToInt(x * scaleX),
        juce::roundToInt(y * scaleY),
        juce::roundToInt(w * scaleX),
        juce::roundToInt(h * scaleY)
    };
}

juce::ImageButton *ObxdAudioProcessorEditor::addMenuButton(const int x, const int y, const int d,
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

void ObxdAudioProcessorEditor::rebuildComponents(ObxdAudioProcessor &ownerFilter)
{
    skinFolder = utils.getCurrentSkinFolder();

    ownerFilter.removeChangeListener(this);

    setSize(1440, 450);

    ownerFilter.addChangeListener(this);
    repaint();

}

void ObxdAudioProcessorEditor::createMenu()
{
    juce::MemoryBlock memoryBlock;
    memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());
    bool enablePasteOption = utils.isMemoryBlockAPreset(memoryBlock);
    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    skins = utils.getSkinFiles();
    banks = utils.getBankFiles(); {
        juce::PopupMenu fileMenu;

        fileMenu.addItem(static_cast<int>(MenuAction::ImportPreset),
                         "Import Preset...",
                         true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::ImportBank),
                         "Import Bank...",
                         true,
                         false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::ExportPreset),
                         "Export Preset...",
                         true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::ExportBank),
                         "Export Bank...",
                         true,
                         false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::NewPreset),
                         "New Preset...",
                         true, //enableNewPresetOption,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::RenamePreset),
                         "Rename Preset...",
                         true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::SavePreset),
                         "Save Preset...",
                         true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::DeletePreset),
                         "Delete Preset...",
                         true,
                         false);

        fileMenu.addSeparator();

        fileMenu.addItem(static_cast<int>(MenuAction::CopyPreset),
                         "Copy Preset...",
                         true,
                         false);

        fileMenu.addItem(static_cast<int>(MenuAction::PastePreset),
                         "Paste Preset...",
                         enablePasteOption,
                         false);

        menu->addSubMenu("File", fileMenu);
    } {
        juce::PopupMenu progMenu;
        for (int i = 0; i < processor.getNumPrograms(); ++i)
        {
            progMenu.addItem(i + progStart + 1,
                             processor.getProgramName(i),
                             true,
                             i == processor.getCurrentProgram());
        }

        menu->addSubMenu("Programs", progMenu);
    }

    menu->addItem(progStart + 1000, "Preset Bar", true, utils.getShowPresetBar()); {
        juce::PopupMenu bankMenu;
        const juce::String currentBank = utils.getCurrentBankFile().getFileName();

        for (int i = 0; i < banks.size(); ++i)
        {
            const juce::File bank = banks.getUnchecked(i);
            bankMenu.addItem(i + bankStart + 1,
                             bank.getFileNameWithoutExtension(),
                             true,
                             bank.getFileName() == currentBank);
        }

        menu->addSubMenu("Banks", bankMenu);
    } {
        juce::PopupMenu skinMenu;
        for (int i = 0; i < skins.size(); ++i)
        {
            const juce::File skin = skins.getUnchecked(i);
            skinMenu.addItem(i + skinStart + 1,
                             skin.getFileName(),
                             true,
                             skin.getFileName() == skinFolder.getFileName());
        }

        menu->addSubMenu("Themes", skinMenu);
    }

    menuMidiNum = progStart + 2000;
    createMidi(menuMidiNum, midiMenu);
    menu->addSubMenu("MIDI", midiMenu);
    popupMenus.add(menu);

    juce::PopupMenu tooltipMenu;
    tooltipMenu.addItem("Disabled", true,
                        utils.getTooltipBehavior() == Tooltip::Disable, [&] {
                            utils.setTooltipBehavior(Tooltip::Disable);
                            resized();
                        });
    tooltipMenu.addItem("Standard Display", true,
                        utils.getTooltipBehavior() == Tooltip::StandardDisplay, [&] {
                            utils.setTooltipBehavior(Tooltip::StandardDisplay);
                            resized();
                        });
    tooltipMenu.addItem("Full Display", true,
                        utils.getTooltipBehavior() == Tooltip::FullDisplay,
                        [&] {
                            utils.setTooltipBehavior(Tooltip::FullDisplay);
                            resized();
                        });
    menu->addSubMenu("Tooltips", tooltipMenu, true);

#ifdef LINUX
    menu->addItem(1, String("Release ") +  String(JucePlugin_VersionString).dropLastCharacters(2), false);
#endif

#if defined(JUCE_MAC) || defined(WIN32)
    juce::PopupMenu helpMenu;
    juce::String version = juce::String("Release ") + juce::String(JucePlugin_VersionString).
                           dropLastCharacters(2);
    helpMenu.addItem(menuScaleNum + 4, "Manual", true);
    helpMenu.addItem(menuScaleNum + 3, version, false);
    menu->addSubMenu("Help", helpMenu, true);
#endif
}


void ObxdAudioProcessorEditor::createMidi(int menuNo, juce::PopupMenu &menuMidi)
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
        if (juce::File f(i); f.getFileNameWithoutExtension() != "Default" && f.
                             getFileNameWithoutExtension() != "Custom" && f.
                             getFileNameWithoutExtension() != "Config")
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

void ObxdAudioProcessorEditor::resultFromMenu(const juce::Point<int> pos)
{
    createMenu();

    if (int result = popupMenus[0]->showAt(juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1));
        result >= (skinStart + 1) && result <= (skinStart + skins.size()))
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
    else if (result >= (progStart + 1) && result <= (progStart + processor.getNumPrograms()))
    {
        result -= 1;
        result -= progStart;
        processor.setCurrentProgram(result);
    }
    else if (result < progStart)
    {
        MenuActionCallback(result);
    }
    else if (result == progStart + 1000)
    {
        utils.setShowPresetBar(!utils.getShowPresetBar());
        updatePresetBar(true);
    }
    else if (result >= menuScaleNum)
    {

        if (result == menuScaleNum + 4)
        {
            const juce::File manualFile = utils.getDocumentFolder().
                getChildFile("OB-Xd Manual.pdf");
            utils.openInPdf(manualFile);
        }
    }
    else if (result >= menuMidiNum)
    {
        if (auto selected_idx = result - menuMidiNum;
            selected_idx < static_cast<decltype(selected_idx)>(midiFiles.size()))
        // Now both operands are the same type
        {
            if (juce::File f(midiFiles[selected_idx]); f.exists())
            {
                processor.getCurrentMidiPath() = midiFiles[selected_idx];
                processor.bindings.loadFile(f);
                processor.updateMidiConfig();
            }
        }
    }
}

void ObxdAudioProcessorEditor::updatePresetBar(const bool resize)
{
    if (utils.getShowPresetBar())
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
                             getHeight() - presetBar->getHeight(),
                             presetBar->getWidth(),
                             presetBar->getHeight());
    }
    else if (presetBar->isVisible())
    {
        if (resize)
        {
            setSize(getWidth(), getHeight() - presetBar->getHeight());
            resized();
            scaleFactorChanged();
            repaint();

            if (const auto parent = getParentComponent())
            {
                parent->resized();
                parent->repaint();
            }
        }
        presetBar->setVisible(false);
    }
}


void ObxdAudioProcessorEditor::MenuActionCallback(int action)
{

    if (action == MenuAction::ImportBank)
    {
        fileChooser = std::make_unique<juce::FileChooser>("Import Bank (*.fxb)", juce::File(),
                                                          "*.fxb", true);

        if (fileChooser->browseForFileToOpen())
        {
            const juce::File result = fileChooser->getResult();
            const auto name = result.getFileName().replace("%20", " ");

            if (const auto file = utils.getBanksFolder().getChildFile(name);
                result == file || result.copyFileTo(file))
            {
                utils.loadFromFXBFile(file);
                utils.scanAndUpdateBanks();
            }
        }
    };

    if (action == MenuAction::ExportBank)
    {
        const auto file = utils.getDocumentFolder().getChildFile("Banks");
        if (juce::FileChooser myChooser("Export Bank (*.fxb)", file, "*.fxb", true); myChooser.
            browseForFileToSave(true))
        {
            const juce::File result = myChooser.getResult();

            juce::String temp = result.getFullPathName();
            if (!temp.endsWith(".fxb"))
            {
                temp += ".fxb";
            }
            utils.saveBank(juce::File(temp));
        }
    };

    if (action == MenuAction::DeleteBank)
    {
        if (juce::NativeMessageBox::showOkCancelBox(juce::AlertWindow::NoIcon, "Delete Bank",
                                                    "Delete current bank: "
                                                    + utils.getCurrentBank() + "?"))
        {
            utils.deleteBank();
        }
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
        if (juce::NativeMessageBox::showOkCancelBox(juce::AlertWindow::NoIcon, "Delete Preset",
                                                    "Delete current preset " + utils.
                                                    getCurrentPreset() + "?"))
        {
            utils.deletePreset();
        }
        return;
    }

    if (action == MenuAction::ImportPreset)
    {
        fileChooser = std::make_unique<juce::FileChooser>("Import Preset (*.fxp)", juce::File(),
                                                          "*.fxp", true);

        if (fileChooser->browseForFileToOpen())
        {
            juce::File result = fileChooser->getResult();
            utils.loadPreset(result);
        }
    };

    if (action == MenuAction::ExportPreset)
    {

        auto file = utils.getPresetsFolder();
        if (juce::FileChooser myChooser("Export Preset (*.fxp)", file, "*.fxp", true); myChooser.
            browseForFileToSave(true))
        {
            juce::File result = myChooser.getResult();

            juce::String temp = result.getFullPathName();
            if (!temp.endsWith(".fxp"))
            {
                temp += ".fxp";
            }
            utils.savePreset(juce::File(temp));

        }
    };

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

void ObxdAudioProcessorEditor::nextProgram()
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

void ObxdAudioProcessorEditor::prevProgram()
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

void ObxdAudioProcessorEditor::buttonClicked(juce::Button *b)
{

    if (const auto toggleButton = dynamic_cast<TooglableButton *>(b);
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


void ObxdAudioProcessorEditor::updateFromHost()
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

void ObxdAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster * /*source*/)
{
    updateFromHost();
}

void ObxdAudioProcessorEditor::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        resultFromMenu(e.getMouseDownScreenPosition());
    }
}


void ObxdAudioProcessorEditor::handleAsyncUpdate()
{
    scaleFactorChanged();
    repaint();
}

void ObxdAudioProcessorEditor::paint(juce::Graphics &g)
{

    const float newPhysicalPixelScaleFactor =
        g.getInternalContext().getPhysicalPixelScaleFactor();

    if (newPhysicalPixelScaleFactor != utils.getPixelScaleFactor())
    {
        utils.setPixelScaleFactor(newPhysicalPixelScaleFactor);
        scaleFactorChanged();
    }

    g.fillAll(juce::Colours::black.brighter(0.1f));

    // background gui
    if (utils.getShowPresetBar())
    {
        g.drawImage(backgroundImage,
                    0, 0, getWidth(), getHeight() - 40,
                    0, 0, backgroundImage.getWidth(), backgroundImage.getHeight());
    }
    else
    {
        g.drawImage(backgroundImage,
                    0, 0, getWidth(), getHeight(),
                    0, 0, backgroundImage.getWidth(), backgroundImage.getHeight());
    }
}


bool ObxdAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray &files)
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

void ObxdAudioProcessorEditor::filesDropped(const juce::StringArray &files, int /*x*/,
                                            int /*y*/)
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

            if (const auto result = utils.getBanksFolder().getChildFile(name); file.
                copyFileTo(result))
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


const juce::String ObxdAudioProcessorEditor::Action::panReset{"panReset"};