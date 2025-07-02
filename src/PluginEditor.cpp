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
#include "Utils.h"

static std::weak_ptr<obxf::LookAndFeel> sharedLookAndFeelWeak;

struct IdleTimer : juce::Timer
{
    ObxfAudioProcessorEditor *editor{nullptr};
    IdleTimer(ObxfAudioProcessorEditor *e) : editor(e) {}
    void timerCallback() override
    {
        if (editor)
        {
            editor->idle();
        }
    }
};

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
            lookAndFeelPtr = std::make_shared<obxf::LookAndFeel>();
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

    int initialWidth = backgroundImage.getWidth();
    int initialHeight = backgroundImage.getHeight();

    if (noThemesAvailable)
    {
        initialWidth = 800;
        initialHeight = 400;
    }

    setSize(initialWidth, initialHeight);

    setOriginalBounds(getLocalBounds());

    setResizable(true, false);

    constrainer = std::make_unique<AspectRatioDownscaleConstrainer>(initialWidth, initialHeight);
    constrainer->setMinimumSize(initialWidth / 4, initialHeight / 4);
    setConstrainer(constrainer.get());

    updateFromHost();

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
}

void ObxfAudioProcessorEditor::resized()
{
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
        if (doc->getTagName() == "obxf-skin")
        {
            for (const auto *child : doc->getChildWithTagNameIterator("object"))
            {
                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");

                if (mappingComps[name] != nullptr)
                {
                    if (dynamic_cast<Label *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }
                }
            }

            for (const auto *child : doc->getChildWithTagNameIterator("parameter"))
            {
                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");
                const auto d = child->getIntAttribute("d");

                if (mappingComps[name] != nullptr)
                {
                    if (auto *knob = dynamic_cast<Knob *>(mappingComps[name]))
                    {
                        if (d > 0)
                        {
                            knob->setBounds(transformBounds(x, y, d, d));
                        }
                        else if (w > 0 && h > 0)
                        {
                            knob->setBounds(transformBounds(x, y, w, h));
                        }
                        else
                        {
                            knob->setBounds(
                                transformBounds(x, y, defKnobDiameter, defKnobDiameter));
                        }

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
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<MidiKeyboard *>(mappingComps[name]))
                    {
                        mappingComps[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<juce::ImageComponent *>(mappingComps[name]))
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
    skins = utils.getSkinFiles();
    if (skins.isEmpty())
    {
        noThemesAvailable = true;
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "No Themes Found",
            "No theme presets were found in the theme directory. The editor cannot be displayed.");
        repaint();
        return;
    }
    noThemesAvailable = false;

    std::map<juce::String, float> parameterValues;
    for (auto &[paramId, component] : mappingComps)
    {
        if (const auto *knob = dynamic_cast<Knob *>(component))
        {
            parameterValues[paramId] = static_cast<float>(knob->getValue());
        }
        else if (const auto *button = dynamic_cast<ToggleButton *>(component))
        {
            parameterValues[paramId] = button->getToggleState() ? 1.0f : 0.0f;
        }
    }

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    menuButton.reset();
    popupMenus.clear();
    mappingComps.clear();
    ownerFilter.removeChangeListener(this);

    skinFolder = utils.getCurrentSkinFolder();
    const juce::File coords = skinFolder.getChildFile("coords.xml");

    if (const bool useClassicSkin = coords.existsAsFile(); !useClassicSkin)
    {
        addMenu(14, 25, 23, 35, "menu");
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
        if (doc->getTagName() == "obxf-skin")
        {
            using namespace SynthParam;

            for (const auto *child : doc->getChildWithTagNameIterator("object"))
            {
                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");
                const auto fh = child->getIntAttribute("fh");

                if (name == "filterModeLabel")
                {
                    if (auto label = addLabel(x, y, w, h, fh, "label-filter-mode");
                        label != nullptr)
                    {
                        filterModeLabel = std::move(label);
                        mappingComps["filterModeLabel"] = filterModeLabel.get();
                    }
                }
            }

            for (const auto *child : doc->getChildWithTagNameIterator("parameter"))
            {

                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");
                const auto d = child->getIntAttribute("d");
                const auto fh = child->getIntAttribute("fh");
                const auto pic = child->getStringAttribute("pic");

                if (name == "midiKeyboard")
                {
                    const auto keyboard = addMidiKeyboard(x, y, w, h);
                    mappingComps["midiKeyboard"] = keyboard;
                }

                if (name == "legatoList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, LEGATOMODE, Name::LegatoMode,
                                            "menu-legato");
                        list != nullptr)
                    {
                        legatoList = std::move(list);
                        mappingComps["legatoList"] = legatoList.get();
                    }
                }

                if (name == "polyphonyList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, POLYPHONY, Name::Polyphony,
                                            "menu-poly");
                        list != nullptr)
                    {
                        polyphonyList = std::move(list);
                        mappingComps["polyphonyList"] = polyphonyList.get();
                    }
                }

                if (name == "unisonVoicesList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, UNISON_VOICES,
                                            Name::UnisonVoices, "menu-voices");
                        list != nullptr)
                    {
                        unisonVoicesList = std::move(list);
                        mappingComps["unisonVoicesList"] = unisonVoicesList.get();
                    }
                }

                if (name == "bendUpRangeList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, PITCH_BEND_UP,
                                            Name::PitchBendUpRange, "menu-pitch-bend");
                        list != nullptr)
                    {
                        bendUpRangeList = std::move(list);
                        mappingComps["bendUpRangeList"] = bendUpRangeList.get();
                    }
                }

                if (name == "bendDownRangeList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, PITCH_BEND_DOWN,
                                            Name::PitchBendDownRange, "menu-pitch-bend");
                        list != nullptr)
                    {
                        bendDownRangeList = std::move(list);
                        mappingComps["bendDownRangeList"] = bendDownRangeList.get();
                    }
                }

                if (name == "menu")
                {
                    menuButton = addMenu(x, y, w, h, "button-clear-red");
                    mappingComps["menu"] = menuButton.get();
                }

                if (name == "resonanceKnob")
                {
                    resonanceKnob = addKnob(x, y, w, h, d, fh, ownerFilter, RESONANCE, 0.f,
                                            Name::Resonance, useAssetOrDefault(pic, "knob"));
                    mappingComps["resonanceKnob"] = resonanceKnob.get();
                }
                if (name == "cutoffKnob")
                {
                    cutoffKnob = addKnob(x, y, w, h, d, fh, ownerFilter, CUTOFF, 1.f, Name::Cutoff,
                                         useAssetOrDefault(pic, "knob"));
                    mappingComps["cutoffKnob"] = cutoffKnob.get();
                }
                if (name == "filterEnvelopeAmtKnob")
                {
                    filterEnvelopeAmtKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ENVELOPE_AMT, 0.f,
                                Name::FilterEnvAmount, useAssetOrDefault(pic, "knob"));
                    mappingComps["filterEnvelopeAmtKnob"] = filterEnvelopeAmtKnob.get();
                }
                if (name == "multimodeKnob")
                {
                    multimodeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, MULTIMODE, 0.f,
                                            Name::Multimode, useAssetOrDefault(pic, "knob"));
                    mappingComps["multimodeKnob"] = multimodeKnob.get();
                }

                if (name == "volumeKnob")
                {
                    volumeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, VOLUME, 0.5f, Name::Volume,
                                         useAssetOrDefault(pic, "knob"));
                    mappingComps["volumeKnob"] = volumeKnob.get();
                }
                if (name == "portamentoKnob")
                {
                    portamentoKnob = addKnob(x, y, w, h, d, fh, ownerFilter, PORTAMENTO, 0.f,
                                             Name::Portamento, useAssetOrDefault(pic, "knob"));
                    mappingComps["portamentoKnob"] = portamentoKnob.get();
                }
                if (name == "osc1PitchKnob")
                {
                    osc1PitchKnob = addKnob(x, y, w, h, d, fh, ownerFilter, OSC1P, 0.5f,
                                            Name::Osc1Pitch, useAssetOrDefault(pic, "knob"));
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
                    pulseWidthKnob = addKnob(x, y, w, h, d, fh, ownerFilter, PW, 0.f,
                                             Name::PulseWidth, useAssetOrDefault(pic, "knob"));
                    mappingComps["pulseWidthKnob"] = pulseWidthKnob.get();
                }
                if (name == "osc2PitchKnob")
                {
                    osc2PitchKnob = addKnob(x, y, w, h, d, fh, ownerFilter, OSC2P, 0.5f,
                                            Name::Osc2Pitch, useAssetOrDefault(pic, "knob"));
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

                if (name == "osc1MixKnob")
                {
                    osc1MixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, OSC1MIX, 1.f,
                                          Name::Osc1Mix, useAssetOrDefault(pic, "knob"));
                    mappingComps["osc1MixKnob"] = osc1MixKnob.get();
                }
                if (name == "osc2MixKnob")
                {
                    osc2MixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, OSC2MIX, 1.f,
                                          Name::Osc2Mix, useAssetOrDefault(pic, "knob"));
                    mappingComps["osc2MixKnob"] = osc2MixKnob.get();
                }
                if (name == "ringModMixKnob")
                {
                    ringModMixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, RINGMODMIX, 0.f,
                                             Name::RingModMix, useAssetOrDefault(pic, "knob"));
                    mappingComps["ringModMixKnob"] = ringModMixKnob.get();
                }
                if (name == "noiseMixKnob")
                {
                    noiseMixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, NOISEMIX, 0.f,
                                           Name::NoiseMix, useAssetOrDefault(pic, "knob"));
                    mappingComps["noiseMixKnob"] = noiseMixKnob.get();
                }
                if (name == "noiseColorButton")
                {
                    noiseColorButton =
                        addButton(x, y, w, h, ownerFilter, NOISE_COLOR, Name::NoiseColor,
                                  useAssetOrDefault(pic, "button-slim-noise"));
                    mappingComps["noiseColorButton"] = noiseColorButton.get();
                }

                if (name == "xmodKnob")
                {
                    xmodKnob = addKnob(x, y, w, h, d, fh, ownerFilter, XMOD, 0.f, Name::Xmod,
                                       useAssetOrDefault(pic, "knob"));
                    mappingComps["xmodKnob"] = xmodKnob.get();
                }
                if (name == "osc2DetuneKnob")
                {
                    osc2DetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, OSC2_DET, 0.f,
                                Name::Oscillator2Detune, useAssetOrDefault(pic, "knob"));
                    mappingComps["osc2DetuneKnob"] = osc2DetuneKnob.get();
                }

                if (name == "envPitchModKnob")
                {
                    envPitchModKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ENVPITCH, 0.f,
                                Name::EnvelopeToPitch, useAssetOrDefault(pic, "knob"));
                    mappingComps["envPitchModKnob"] = envPitchModKnob.get();
                }
                if (name == "brightnessKnob")
                {
                    brightnessKnob = addKnob(x, y, w, h, d, fh, ownerFilter, BRIGHTNESS, 1.f,
                                             Name::Brightness, useAssetOrDefault(pic, "knob"));
                    mappingComps["brightnessKnob"] = brightnessKnob.get();
                }

                if (name == "attackKnob")
                {
                    attackKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LATK, 0.f, Name::Attack,
                                         useAssetOrDefault(pic, "knob"));
                    mappingComps["attackKnob"] = attackKnob.get();
                }
                if (name == "decayKnob")
                {
                    decayKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LDEC, 0.f, Name::Decay,
                                        useAssetOrDefault(pic, "knob"));
                    mappingComps["decayKnob"] = decayKnob.get();
                }
                if (name == "sustainKnob")
                {
                    sustainKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LSUS, 1.f, Name::Sustain,
                                          useAssetOrDefault(pic, "knob"));
                    mappingComps["sustainKnob"] = sustainKnob.get();
                }
                if (name == "releaseKnob")
                {
                    releaseKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LREL, 0.f, Name::Release,
                                          useAssetOrDefault(pic, "knob"));
                    mappingComps["releaseKnob"] = releaseKnob.get();
                }

                if (name == "fattackKnob")
                {
                    fattackKnob = addKnob(x, y, w, h, d, fh, ownerFilter, FATK, 0.f,
                                          Name::FilterAttack, useAssetOrDefault(pic, "knob"));
                    mappingComps["fattackKnob"] = fattackKnob.get();
                }
                if (name == "fdecayKnob")
                {
                    fdecayKnob = addKnob(x, y, w, h, d, fh, ownerFilter, FDEC, 0.f,
                                         Name::FilterDecay, useAssetOrDefault(pic, "knob"));
                    mappingComps["fdecayKnob"] = fdecayKnob.get();
                }
                if (name == "fsustainKnob")
                {
                    fsustainKnob = addKnob(x, y, w, h, d, fh, ownerFilter, FSUS, 1.f,
                                           Name::FilterSustain, useAssetOrDefault(pic, "knob"));
                    mappingComps["fsustainKnob"] = fsustainKnob.get();
                }
                if (name == "freleaseKnob")
                {
                    freleaseKnob = addKnob(x, y, w, h, d, fh, ownerFilter, FREL, 0.f,
                                           Name::FilterRelease, useAssetOrDefault(pic, "knob"));
                    mappingComps["freleaseKnob"] = freleaseKnob.get();
                }

                if (name == "lfoFrequencyKnob")
                {
                    lfoFrequencyKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, LFOFREQ, 0.5f, Name::LfoFrequency,
                                useAssetOrDefault(pic, "knob")); // 4 Hz
                    mappingComps["lfoFrequencyKnob"] = lfoFrequencyKnob.get();
                }
                if (name == "lfoAmt1Knob")
                {
                    lfoAmt1Knob = addKnob(x, y, w, h, d, fh, ownerFilter, LFO1AMT, 0.f,
                                          Name::LfoAmount1, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoAmt1Knob"] = lfoAmt1Knob.get();
                }
                if (name == "lfoAmt2Knob")
                {
                    lfoAmt2Knob = addKnob(x, y, w, h, d, fh, ownerFilter, LFO2AMT, 0.f,
                                          Name::LfoAmount2, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoAmt2Knob"] = lfoAmt2Knob.get();
                }

                if (name == "lfoWave1Knob")
                {
                    lfoWave1Knob = addKnob(x, y, w, h, d, fh, ownerFilter, LFOSINWAVE, 0.5f,
                                           Name::LfoSineWave, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoWave1Knob"] = lfoWave1Knob.get();
                }
                if (name == "lfoWave2Knob")
                {
                    lfoWave2Knob = addKnob(x, y, w, h, d, fh, ownerFilter, LFOSQUAREWAVE, 0.5f,
                                           Name::LfoSquareWave, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoWave2Knob"] = lfoWave2Knob.get();
                }
                if (name == "lfoWave3Knob")
                {
                    lfoWave3Knob = addKnob(x, y, w, h, d, fh, ownerFilter, LFOSHWAVE, 0.5f,
                                           Name::LfoSampleHoldWave, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoWave3Knob"] = lfoWave3Knob.get();
                }

                if (name == "lfoPWKnob")
                {
                    lfoPWKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LFOPW, 0.f,
                                        Name::LfoPulsewidth, useAssetOrDefault(pic, "knob"));
                    mappingComps["lfoPWKnob"] = lfoPWKnob.get();
                }

                if (name == "lfoOsc1Button")
                {
                    lfoOsc1Button = addButton(x, y, w, h, ownerFilter, LFOOSC1, Name::LfoOsc1,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["lfoOsc1Button"] = lfoOsc1Button.get();
                }
                if (name == "lfoOsc2Button")
                {
                    lfoOsc2Button = addButton(x, y, w, h, ownerFilter, LFOOSC2, Name::LfoOsc2,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["lfoOsc2Button"] = lfoOsc2Button.get();
                }
                if (name == "lfoFilterButton")
                {
                    lfoFilterButton = addButton(x, y, w, h, ownerFilter, LFOFILTER, Name::LfoFilter,
                                                useAssetOrDefault(pic, "button"));
                    mappingComps["lfoFilterButton"] = lfoFilterButton.get();
                }

                if (name == "lfoPwm1Button")
                {
                    lfoPwm1Button = addButton(x, y, w, h, ownerFilter, LFOPW1, Name::LfoPw1,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["lfoPwm1Button"] = lfoPwm1Button.get();
                }
                if (name == "lfoPwm2Button")
                {
                    lfoPwm2Button = addButton(x, y, w, h, ownerFilter, LFOPW2, Name::LfoPw2,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["lfoPwm2Button"] = lfoPwm2Button.get();
                }
                if (name == "lfoVolumeButton")
                {
                    lfoVolumeButton = addButton(x, y, w, h, ownerFilter, LFOVOLUME, Name::LfoVolume,
                                                useAssetOrDefault(pic, "button"));
                    mappingComps["lfoVolumeButton"] = lfoVolumeButton.get();
                }

                if (name == "hardSyncButton")
                {
                    hardSyncButton = addButton(x, y, w, h, ownerFilter, OSC2HS, Name::Osc2HardSync,
                                               useAssetOrDefault(pic, "button"));
                    mappingComps["hardSyncButton"] = hardSyncButton.get();
                }
                if (name == "osc1SawButton")
                {
                    osc1SawButton = addButton(x, y, w, h, ownerFilter, OSC1Saw, Name::Osc1Saw,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["osc1SawButton"] = osc1SawButton.get();
                }
                if (name == "osc2SawButton")
                {
                    osc2SawButton = addButton(x, y, w, h, ownerFilter, OSC2Saw, Name::Osc2Saw,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["osc2SawButton"] = osc2SawButton.get();
                }

                if (name == "osc1PulButton")
                {
                    osc1PulButton = addButton(x, y, w, h, ownerFilter, OSC1Pul, Name::Osc1Pulse,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["osc1PulButton"] = osc1PulButton.get();
                }
                if (name == "osc2PulButton")
                {
                    osc2PulButton = addButton(x, y, w, h, ownerFilter, OSC2Pul, Name::Osc2Pulse,
                                              useAssetOrDefault(pic, "button"));
                    mappingComps["osc2PulButton"] = osc2PulButton.get();
                }

                if (name == "pitchEnvInvertButton")
                {
                    pitchEnvInvertButton = addButton(x, y, w, h, ownerFilter, ENVPITCHINV,
                                                     Name::EnvelopeToPitchInv, "button-slim");
                    mappingComps["pitchEnvInvertButton"] = pitchEnvInvertButton.get();
                }

                if (name == "pwEnvInvertButton")
                {
                    pwEnvInvertButton = addButton(x, y, w, h, ownerFilter, ENVPWINV,
                                                  Name::EnvelopeToPWInv, "button-slim");
                    mappingComps["pwEnvInvertButton"] = pwEnvInvertButton.get();
                }

                if (name == "filterBPBlendButton")
                {
                    filterBPBlendButton = addButton(x, y, w, h, ownerFilter, BANDPASS,
                                                    Name::BandpassBlend, "button-slim");
                    mappingComps["filterBPBlendButton"] = filterBPBlendButton.get();
                }
                if (name == "fourPoleButton")
                {
                    fourPoleButton =
                        addButton(x, y, w, h, ownerFilter, FOURPOLE, Name::FourPole, "button-slim");
                    mappingComps["fourPoleButton"] = fourPoleButton.get();
                }
                if (name == "oversamplingButton")
                {
                    oversamplingButton = addButton(x, y, w, h, ownerFilter, FILTER_WARM,
                                                   Name::HQMode, useAssetOrDefault(pic, "button"));
                    mappingComps["oversamplingButton"] = oversamplingButton.get();
                }

                if (name == "filterKeyFollowKnob")
                {
                    filterKeyFollowKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, FLT_KF, 0.f, Name::FilterKeyFollow,
                                useAssetOrDefault(pic, "knob"));
                    mappingComps["filterKeyFollowKnob"] = filterKeyFollowKnob.get();
                }

                if (name == "unisonButton")
                {
                    unisonButton = addButton(x, y, w, h, ownerFilter, UNISON, Name::Unison,
                                             useAssetOrDefault(pic, "button"));
                    mappingComps["unisonButton"] = unisonButton.get();
                }

                if (name == "tuneKnob")
                {
                    tuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, TUNE, 0.5f, Name::Tune,
                                       useAssetOrDefault(pic, "knob"));
                    mappingComps["tuneKnob"] = tuneKnob.get();
                }
                if (name == "transposeKnob")
                {
                    transposeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, OCTAVE, 0.5f,
                                            Name::Transpose, useAssetOrDefault(pic, "knob"));
                    transposeKnob->cmdDragCallback = [](const double value) {
                        const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
                        return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0,
                                          1.0);
                    };
                    transposeKnob->altDragCallback = [](const double value) {
                        const auto octValue = (int)juce::jmap(value, -2.0, 2.0);
                        return juce::jmap((double)octValue, -2.0, 2.0, 0.0, 1.0);
                    };

                    mappingComps["transposeKnob"] = transposeKnob.get();
                }

                if (name == "voiceDetuneKnob")
                {
                    voiceDetuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, UDET, 0.25f,
                                              Name::VoiceDetune, useAssetOrDefault(pic, "knob"));
                    mappingComps["voiceDetuneKnob"] = voiceDetuneKnob.get();
                }

                if (name == "vibratoWaveButton")
                {
                    vibratoWaveButton =
                        addButton(x, y, w, h, ownerFilter, BENDLFOWAVE, Name::VibratoRate,
                                  useAssetOrDefault(pic, "button-slim"));
                    mappingComps["vibratoWaveButton"] = vibratoWaveButton.get();
                }
                if (name == "vibratoRateKnob")
                {
                    vibratoRateKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, BENDLFORATE, 0.2f,
                                Name::VibratoRate, useAssetOrDefault(pic, "knob")); // 4 Hz
                    mappingComps["vibratoRateKnob"] = vibratoRateKnob.get();
                }

                if (name == "veloFltEnvKnob")
                {
                    veloFltEnvKnob = addKnob(x, y, w, h, d, fh, ownerFilter, VFLTENV, 0.f,
                                             Name::VFltFactor, useAssetOrDefault(pic, "knob"));
                    mappingComps["veloFltEnvKnob"] = veloFltEnvKnob.get();
                }
                if (name == "veloAmpEnvKnob")
                {
                    veloAmpEnvKnob = addKnob(x, y, w, h, d, fh, ownerFilter, VAMPENV, 0.f,
                                             Name::VAmpFactor, useAssetOrDefault(pic, "knob"));
                    mappingComps["veloAmpEnvKnob"] = veloAmpEnvKnob.get();
                }
                if (name == "midiLearnButton")
                {
                    midiLearnButton = addButton(x, y, w, h, ownerFilter, -1, Name::MidiLearn,
                                                useAssetOrDefault(pic, "button"));
                    mappingComps["midiLearnButton"] = midiLearnButton.get();
                    midiLearnButton->onClick = [this]() {
                        const bool state = midiLearnButton->getToggleState();
                        paramManager.midiLearnAttachment.set(state);
                    };
                }
                if (name == "midiUnlearnButton")
                {
                    midiUnlearnButton = addButton(x, y, w, h, ownerFilter, -1, Name::MidiUnlearn,
                                                  useAssetOrDefault(pic, "button-clear"));
                    mappingComps["midiUnlearnButton"] = midiUnlearnButton.get();
                    midiUnlearnButton->onClick = [this]() {
                        const bool state = midiUnlearnButton->getToggleState();
                        paramManager.midiUnlearnAttachment.set(state);
                    };
                }

                if (name.startsWith("pan") && name.endsWith("Knob"))
                {
                    auto which = name.retainCharacters("12345678").getIntValue();
                    auto whichIdx = which - 1;

                    if (whichIdx >= 0 && whichIdx < MAX_PANNINGS)
                    {
                        panKnobs[whichIdx] = addKnob(
                            x, y, w, h, d, fh, ownerFilter, PAN1 + whichIdx, 0.5f,
                            fmt::format("Pan Voice {}", which), useAssetOrDefault(pic, "knob"));
                        mappingComps[fmt::format("pan{}Knob", which)] = panKnobs[whichIdx].get();
                    }
                }

                if (name == "bendOsc2OnlyButton")
                {
                    bendOsc2OnlyButton =
                        addButton(x, y, w, h, ownerFilter, BENDOSC2, Name::BendOsc2Only,
                                  useAssetOrDefault(pic, "button"));
                    mappingComps["bendOsc2OnlyButton"] = bendOsc2OnlyButton.get();
                }

                if (name == "notePriorityButton")
                {
                    notePriorityButton =
                        addButton(x, y, w, h, ownerFilter, NOTE_PRIORITY_MODE, Name::NotePriority,
                                  useAssetOrDefault(pic, "button"));
                    mappingComps["notePriorityButton"] = notePriorityButton.get();
                }

                if (name == "filterDetuneKnob")
                {
                    filterDetuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, FILTERDER, 0.25f,
                                               Name::FilterDetune, useAssetOrDefault(pic, "knob"));
                    mappingComps["filterDetuneKnob"] = filterDetuneKnob.get();
                }
                if (name == "portamentoDetuneKnob")
                {
                    portamentoDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, PORTADER, 0.25f,
                                Name::PortamentoDetune, useAssetOrDefault(pic, "knob"));
                    mappingComps["portamentoDetuneKnob"] = portamentoDetuneKnob.get();
                }
                if (name == "envelopeDetuneKnob")
                {
                    envelopeDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ENVDER, 0.25f, Name::EnvelopeDetune,
                                useAssetOrDefault(pic, "knob"));
                    mappingComps["envelopeDetuneKnob"] = envelopeDetuneKnob.get();
                }
                if (name == "volumeDetuneKnob")
                {
                    volumeDetuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, LEVEL_DIF, 0.25f,
                                               Name::LevelDetune, useAssetOrDefault(pic, "knob"));
                    mappingComps["volumeDetuneKnob"] = volumeDetuneKnob.get();
                }
                if (name == "lfoSyncButton")
                {
                    lfoSyncButton = addButton(x, y, w, h, ownerFilter, LFO_SYNC, Name::LfoSync,
                                              useAssetOrDefault(pic, "button-slim"));
                    mappingComps["lfoSyncButton"] = lfoSyncButton.get();
                }
                if (name == "pwEnvKnob")
                {
                    pwEnvKnob = addKnob(x, y, w, h, d, fh, ownerFilter, PW_ENV, 0.f, Name::PwEnv,
                                        useAssetOrDefault(pic, "knob"));
                    mappingComps["pwEnvKnob"] = pwEnvKnob.get();
                }
                if (name == "pwEnvBothButton")
                {
                    pwEnvBothButton =
                        addButton(x, y, w, h, ownerFilter, PW_ENV_BOTH, Name::PwEnvBoth,
                                  useAssetOrDefault(pic, "button-slim"));
                    mappingComps["pwEnvBothButton"] = pwEnvBothButton.get();
                }
                if (name == "envPitchBothButton")
                {
                    envPitchBothButton =
                        addButton(x, y, w, h, ownerFilter, ENV_PITCH_BOTH, Name::EnvPitchBoth,
                                  useAssetOrDefault(pic, "button-slim"));
                    mappingComps["envPitchBothButton"] = envPitchBothButton.get();
                }
                if (name == "fenvInvertButton")
                {
                    fenvInvertButton =
                        addButton(x, y, w, h, ownerFilter, FENV_INVERT, Name::FenvInvert,
                                  useAssetOrDefault(pic, "button-slim"));
                    mappingComps["fenvInvertButton"] = fenvInvertButton.get();
                }
                if (name == "pwOffsetKnob")
                {
                    pwOffsetKnob = addKnob(x, y, w, h, d, fh, ownerFilter, PW_OSC2_OFS, 0.f,
                                           Name::PwOsc2Ofs, useAssetOrDefault(pic, "knob"));
                    mappingComps["pwOffsetKnob"] = pwOffsetKnob.get();
                }
                if (name == "selfOscPushButton")
                {
                    selfOscPushButton =
                        addButton(x, y, w, h, ownerFilter, SELF_OSC_PUSH, Name::SelfOscPush,
                                  useAssetOrDefault(pic, "button-slim"));
                    mappingComps["selfOscPushButton"] = selfOscPushButton.get();
                }

                if (name == "prevPatchButton")
                {
                    prevPatchButton = addButton(x, y, w, h, ownerFilter, -1, Name::PrevPatch,
                                                useAssetOrDefault(pic, "button-clear"));
                    mappingComps["prevPatchButton"] = prevPatchButton.get();
                    prevPatchButton->onClick = [this]() { prevProgram(); };
                }
                if (name == "nextPatchButton")
                {
                    nextPatchButton = addButton(x, y, w, h, ownerFilter, -1, Name::NextPatch,
                                                useAssetOrDefault(pic, "button-clear"));
                    mappingComps["nextPatchButton"] = nextPatchButton.get();
                    nextPatchButton->onClick = [this]() { nextProgram(); };
                }

                if (name == "initPatchButton")
                {
                    initPatchButton = addButton(x, y, w, h, ownerFilter, -1, Name::InitPatch,
                                                useAssetOrDefault(pic, "button-clear-red"));
                    mappingComps["initPatchButton"] = initPatchButton.get();
                    initPatchButton->onClick = [this]() {
                        MenuActionCallback(MenuAction::InitPatch);
                    };
                }
                if (name == "randomizePatchButton")
                {
                    randomizePatchButton =
                        addButton(x, y, w, h, ownerFilter, -1, Name::RandomizePatch,
                                  useAssetOrDefault(pic, "button-clear-white"));
                    mappingComps["randomizePatchButton"] = randomizePatchButton.get();
                    /*                     randomizePatchButton->onClick = [this]() {
                                            MenuActionCallback(MenuAction::RandomizePatch);
                                        }; */
                }

                if (name == "groupSelectButton")
                {
                    groupSelectButton =
                        addButton(x, y, w, h, ownerFilter, -1, Name::PatchGroupSelect,
                                  useAssetOrDefault(pic, "button-alt"));
                    mappingComps["groupSelectButton"] = groupSelectButton.get();
                    // TODO implement what it needs to do
                }

                if (name.startsWith("select") && name.endsWith("Button"))
                {
                    auto which = name.retainCharacters("0123456789").getIntValue();
                    auto whichIdx = which - 1;

                    if (whichIdx >= 0 && whichIdx < NUM_PATCHES_PER_GROUP)
                    {
                        selectButtons[whichIdx] =
                            addButton(x, y, w, h, ownerFilter, -1,
                                      fmt::format("Select Group/Patch {}", which),
                                      useAssetOrDefault(pic, "button-group-patch"));
                        mappingComps[fmt::format("select{}Button", which)] =
                            selectButtons[whichIdx].get();
                        // TODO implement what these need to do
                    }
                }
            }
        }

        resized();
    }

    // Prepare data
    if (polyphonyList)
    {
        auto *menu = polyphonyList->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            if (i > 1 && ((1 - i) % (MAX_VOICES / NUM_COLUMNS) == 0))
            {
                menu->addColumnBreak();
            }

            polyphonyList->addChoice(juce::String(i));
        }

        const auto polyOption = ownerFilter.getValueTreeState()
                                    .getParameter(paramManager.getEngineParameterId(POLYPHONY))
                                    ->getValue();
        polyphonyList->setScrollWheelEnabled(true);
        polyphonyList->setValue(polyOption, juce::dontSendNotification);
    }

    if (unisonVoicesList)
    {
        for (int i = 1; i <= MAX_PANNINGS; ++i)
        {
            unisonVoicesList->addChoice(juce::String(i));
        }

        const auto uniVoicesOption =
            ownerFilter.getValueTreeState()
                .getParameter(paramManager.getEngineParameterId(UNISON_VOICES))
                ->getValue();
        unisonVoicesList->setScrollWheelEnabled(true);
        unisonVoicesList->setValue(uniVoicesOption, juce::dontSendNotification);
    }

    if (legatoList)
    {
        legatoList->addChoice("Keep All");
        legatoList->addChoice("Keep Filter Envelope");
        legatoList->addChoice("Keep Amplitude Envelope");
        legatoList->addChoice("Retrigger");
        const auto legatoOption = ownerFilter.getValueTreeState()
                                      .getParameter(paramManager.getEngineParameterId(LEGATOMODE))
                                      ->getValue();
        legatoList->setScrollWheelEnabled(true);
        legatoList->setValue(legatoOption, juce::dontSendNotification);
    }

    if (bendUpRangeList)
    {
        auto *menu = bendUpRangeList->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendUpRangeList->addChoice(juce::String(i));
        }

        const auto voiceOption = ownerFilter.getValueTreeState()
                                     .getParameter(paramManager.getEngineParameterId(PITCH_BEND_UP))
                                     ->getValue();
        bendUpRangeList->setScrollWheelEnabled(true);
        bendUpRangeList->setValue(voiceOption, juce::dontSendNotification);
    }

    if (bendDownRangeList)
    {
        auto *menu = bendDownRangeList->getRootMenu();
        const uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendDownRangeList->addChoice(juce::String(i));
        }

        const auto voiceOption =
            ownerFilter.getValueTreeState()
                .getParameter(paramManager.getEngineParameterId(PITCH_BEND_DOWN))
                ->getValue();
        bendDownRangeList->setScrollWheelEnabled(true);
        bendDownRangeList->setValue(voiceOption, juce::dontSendNotification);
    }

    createMenu();

    for (auto &[paramId, paramValue] : parameterValues)
    {
        if (mappingComps.find(paramId) != mappingComps.end())
        {
            Component *comp = mappingComps[paramId];

            if (auto *knob = dynamic_cast<Knob *>(comp))
            {
                knob->setValue(paramValue, juce::dontSendNotification);
            }
            else if (auto *button = dynamic_cast<ToggleButton *>(comp))
            {
                button->setToggleState(paramValue > 0.5f, juce::dontSendNotification);
            }
        }
    }

    ownerFilter.addChangeListener(this);

    scaleFactorChanged();
    repaint();
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
    idleTimer->stopTimer();
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    menuButton.reset();
    popupMenus.clear();
    mappingComps.clear();

    juce::PopupMenu::dismissAllActiveMenus();
}

void ObxfAudioProcessorEditor::idle()
{
    // DBG("Idle Called");
}

void ObxfAudioProcessorEditor::scaleFactorChanged()
{
    backgroundImage = getScaledImageFromCache("background");
    resized();
}

std::unique_ptr<Label> ObxfAudioProcessorEditor::addLabel(const int x, const int y, const int w,
                                                          const int h, const int fh,
                                                          const juce::String &assetName)
{
    auto *label = new Label(assetName, fh, &processor);

    label->setDrawableBounds(transformBounds(x, y, w, h));
    label->setName(assetName);
    label->toFront(false);

    addAndMakeVisible(label);

    return std::unique_ptr<Label>(label);
}

std::unique_ptr<ButtonList>
ObxfAudioProcessorEditor::addList(const int x, const int y, const int w, const int h,
                                  ObxfAudioProcessor &filter, const int parameter,
                                  const juce::String &name, const juce::String &assetName)
{
#if JUCE_WINDOWS || JUCE_LINUX
    auto *bl = new ButtonList((assetName), h, &processor);
#else
    auto *bl = new ButtonList(assetName, h, &processor);
#endif

    buttonListAttachments.add(new ButtonList::ButtonListAttachment(
        filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *bl));

    bl->setBounds(x, y, w, h);
    bl->setTitle(name);
    addAndMakeVisible(bl);

    return std::unique_ptr<ButtonList>(bl);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(const int x, const int y, const int w,
                                                        const int h, const int d, const int fh,
                                                        ObxfAudioProcessor &filter,
                                                        const int parameter, const float defval,
                                                        const juce::String &name,
                                                        const juce::String &assetName)
{
    int frameHeight = defKnobDiameter;

    if (d > 0)
    {
        frameHeight = d;
    }
    else if (fh > 0)
    {
        frameHeight = fh;
    }

    const auto knob = new Knob(assetName, frameHeight, &processor);

    knobAttachments.add(new Knob::KnobAttachment(
        filter.getValueTreeState(), paramManager.getEngineParameterId(parameter), *knob));

    knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);

    if (d > 0)
    {
        knob->setBounds(transformBounds(x, y, d, d));
    }
    else if (w > 0 && h > 0)
    {
        knob->setBounds(transformBounds(x, y, w, h));

        if (w > h)
        {
            knob->setSliderStyle(juce::Slider::RotaryHorizontalDrag);
        }
    }
    else
    {
        knob->setBounds(transformBounds(x, y, defKnobDiameter, defKnobDiameter));
    }

    knob->setTextBoxStyle(Knob::NoTextBox, true, 0, 0);
    knob->setRange(0, 1);
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

std::unique_ptr<ToggleButton>
ObxfAudioProcessorEditor::addButton(const int x, const int y, const int w, const int h,
                                    ObxfAudioProcessor &filter, const int parameter,
                                    const juce::String &name, const juce::String &assetName)
{
    auto *button = new ToggleButton(assetName, &processor);

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

    const float scaleX = getWidth() / static_cast<float>(originalBounds.getWidth());
    const float scaleY = getHeight() / static_cast<float>(originalBounds.getHeight());

    return {juce::roundToInt(x * scaleX), juce::roundToInt(y * scaleY),
            juce::roundToInt(w * scaleX), juce::roundToInt(h * scaleY)};
}

juce::String ObxfAudioProcessorEditor::useAssetOrDefault(const juce::String &assetName,
                                                         const juce::String &defaultAssetName) const
{
    if (assetName.isNotEmpty())
        return assetName;
    else
        return defaultAssetName;
}

std::unique_ptr<ImageMenu> ObxfAudioProcessorEditor::addMenu(const int x, const int y, const int w,
                                                             const int h,
                                                             const juce::String &assetName)
{
    auto *menu = new ImageMenu(assetName, &processor);
    menu->setBounds(x, y, w, h);
    menu->setName("Menu");

    menu->onClick = [this]() {
        if (menuButton)
        {
            auto x = menuButton->getScreenX();
            auto y = menuButton->getScreenY();
            auto dx = menuButton->getWidth();
            auto pos = juce::Point<int>(x, y + dx);

            resultFromMenu(pos);
        }
    };

    addAndMakeVisible(menu);

    return std::unique_ptr<ImageMenu>(menu);
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
        const uint8_t NUM_COLUMNS = 8;

        juce::PopupMenu patchMenu;
        uint8_t sectionCount = 0;

        for (int i = 0; i < processor.getNumPrograms(); ++i)
        {
            if (i > 0 && i % (processor.getNumPrograms() / NUM_COLUMNS) == 0)
            {
                patchMenu.addColumnBreak();
            }

            if (i % NUM_PATCHES_PER_GROUP == 0)
            {
                sectionCount++;
                patchMenu.addSectionHeader(fmt::format("Group {:d}", sectionCount));
            }

            patchMenu.addItem(i + presetStart + 1,
                              juce::String{i + 1}.paddedLeft('0', 3) + ": " +
                                  processor.getProgramName(i),
                              true, i == processor.getCurrentProgram());
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
    int cur = (processor.getCurrentProgram() + 1) % processor.getNumPrograms();

    processor.setCurrentProgram(cur, false);

    needNotifytoHost = true;
    countTimer = 0;
}

void ObxfAudioProcessorEditor::prevProgram()
{
    int cur = (processor.getCurrentProgram() + processor.getNumPrograms() - 1) %
              processor.getNumPrograms();

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
            processor.getMidiMap().reset();
            processor.getMidiMap().set_default();
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
    if (noThemesAvailable)
    {
        g.fillAll(juce::Colours::red);
        return;
    }

    const float newPhysicalPixelScaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (newPhysicalPixelScaleFactor != utils.getPixelScaleFactor())
    {
        utils.setPixelScaleFactor(newPhysicalPixelScaleFactor);
        scaleFactorChanged();
    }

    g.fillAll(juce::Colours::black.brighter(0.1f));

    // background gui
    g.drawImage(backgroundImage, 0, 0, getWidth(), getHeight(), 0, 0, backgroundImage.getWidth(),
                backgroundImage.getHeight());
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