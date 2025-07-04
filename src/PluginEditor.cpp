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
      paramManager(p.getParamAdaptor()), themeFolder(utils.getThemeFolder()), midiStart(5000),
      sizeStart(4000), presetStart(3000), bankStart(2000), themeStart(1000),
      themes(utils.getThemeFiles()), banks(utils.getBankFiles())
{
    skinLoaded = false;

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

    // fix ProTools file dialog focus issues
    if (!juce::PluginHostType().isProTools())
    {
        startTimer(100);
    };

    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Jersey10_ttf,
                                                            BinaryData::Jersey10_ttfSize);
    patchNameFont = juce::FontOptions(typeface);

    loadTheme(processor);

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

#if defined(DEBUG) || defined(_DEBUG)
    inspector = std::make_unique<melatonin::Inspector>(*this);
#endif
}

void ObxfAudioProcessorEditor::resized()
{
    themeFolder = utils.getCurrentThemeFolder();
    const juce::File theme = themeFolder.getChildFile("theme.xml");

    if (!theme.existsAsFile())
    {
        return;
    }

    juce::XmlDocument themeXml(theme);

    if (const auto doc = themeXml.getDocumentElement())
    {
        if (doc->getTagName() == "obxf-theme")
        {
            for (const auto *child : doc->getChildWithTagNameIterator("widget"))
            {
                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");
                const auto d = child->getIntAttribute("d");

                if (componentMap[name] != nullptr)
                {
                    if (dynamic_cast<Label *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<Display *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (auto *knob = dynamic_cast<Knob *>(componentMap[name]))
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
                    else if (dynamic_cast<ButtonList *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<ImageMenu *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<MultiStateButton *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                    else if (dynamic_cast<ToggleButton *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));

                        if (name.startsWith("select") && name.endsWith("Button"))
                        {
                            componentMap[name.replace("Button", "Label")]->setBounds(
                                transformBounds(x, y, w, h));
                        }
                    }
                    else if (dynamic_cast<MidiKeyboard *>(componentMap[name]))
                    {
                        componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    }
                }
            }
        }
    }
}

void ObxfAudioProcessorEditor::updateSelectButtonStates()
{
    const uint8_t curGroup = processor.getCurrentProgram() / 16;
    const uint8_t curPatchInGroup = processor.getCurrentProgram() % 16;

    for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
    {
        uint8_t offset = 0;

        if (selectButtons[i]->isDown())
            offset += 1;

        if (i == curGroup)
            offset += 2;

        if (i == curPatchInGroup)
            offset += 4;

        selectLabels[i]->setCurrentFrame(offset);
    }
}

void ObxfAudioProcessorEditor::loadTheme(ObxfAudioProcessor &ownerFilter)
{
    themes = utils.getThemeFiles();
    if (themes.isEmpty())
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
    for (auto &[paramId, component] : componentMap)
    {
        if (const auto *knob = dynamic_cast<Knob *>(component))
        {
            parameterValues[paramId] = static_cast<float>(knob->getValue());
        }
        else if (const auto *multiState = dynamic_cast<MultiStateButton *>(component))
        {
            parameterValues[paramId] = static_cast<float>(multiState->getValue());
        }
        else if (const auto *button = dynamic_cast<ToggleButton *>(component))
        {
            parameterValues[paramId] = button->getToggleState() ? 1.0f : 0.0f;
        }
    }

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    menuButton.reset();
    popupMenus.clear();
    componentMap.clear();
    ownerFilter.removeChangeListener(this);

    themeFolder = utils.getCurrentThemeFolder();
    const juce::File theme = themeFolder.getChildFile("theme.xml");

    if (const bool useClassicTheme = theme.existsAsFile(); !useClassicTheme)
    {
        addMenu(14, 25, 23, 35, "button-clear-red");
        rebuildComponents(processor);
        return;
    }

    juce::XmlDocument themeXml(theme);
    if (const auto doc = themeXml.getDocumentElement(); !doc)
    {
        notLoadTheme = true;
        setSize(1440, 486);
    }
    else
    {
        if (doc->getTagName() == "obxf-theme")
        {
            using namespace SynthParam;

            for (const auto *child : doc->getChildWithTagNameIterator("widget"))
            {
                juce::String name = child->getStringAttribute("name");

                const auto x = child->getIntAttribute("x");
                const auto y = child->getIntAttribute("y");
                const auto w = child->getIntAttribute("w");
                const auto h = child->getIntAttribute("h");
                const auto d = child->getIntAttribute("d");
                const auto fh = child->getIntAttribute("fh");
                const auto pic = child->getStringAttribute("pic");

                if (name == "patchNameLabel")
                {
                    patchNameLabel = std::make_unique<Display>("Patch Name");

                    patchNameLabel->setBounds(transformBounds(x, y, w, h));
                    patchNameLabel->setJustificationType(juce::Justification::centred);
                    patchNameLabel->setMinimumHorizontalScale(1.f);
                    patchNameLabel->setFont(patchNameFont.withHeight(20));

                    patchNameLabel->setColour(juce::Label::textColourId, juce::Colours::red);
                    patchNameLabel->setColour(juce::Label::textWhenEditingColourId,
                                              juce::Colours::red);
                    patchNameLabel->setColour(juce::Label::outlineWhenEditingColourId,
                                              juce::Colours::transparentBlack);
                    patchNameLabel->setColour(juce::TextEditor::textColourId, juce::Colours::red);
                    patchNameLabel->setColour(juce::TextEditor::highlightedTextColourId,
                                              juce::Colours::red);
                    patchNameLabel->setColour(juce::TextEditor::highlightColourId,
                                              juce::Colour(0x30FFFFFF));
                    patchNameLabel->setColour(juce::CaretComponent::caretColourId,
                                              juce::Colours::red);

                    patchNameLabel->setVisible(true);

                    addChildComponent(*patchNameLabel);

                    patchNameLabel->onTextChange = [this]() {
                        processor.changeProgramName(processor.getCurrentProgram(),
                                                    patchNameLabel->getText());
                    };

                    componentMap[name] = patchNameLabel.get();
                }

                if (name.startsWith("voice") && name.endsWith("LED"))
                {
                    auto which = name.retainCharacters("0123456789").getIntValue();
                    auto whichIdx = which - 1;

                    if (whichIdx >= 0 && whichIdx < MAX_VOICES)
                    {
                        if (auto label =
                                addLabel(x, y, w, h, h, fmt::format("Voice {} LED", which),
                                         useAssetOrDefault(pic, fmt::format("label-led{}",
                                                                            which / MAX_PANNINGS)));
                            label != nullptr)
                        {
                            voiceLEDs[whichIdx] = std::move(label);
                            componentMap[name] = voiceLEDs[whichIdx].get();
                        }
                    }
                }

                if (name == "midiKeyboard")
                {
                    const auto keyboard = addMidiKeyboard(x, y, w, h);
                    componentMap[name] = keyboard;
                }

                if (name == "legatoList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::EnvLegatoMode,
                                            Name::EnvLegatoMode, "menu-legato");
                        list != nullptr)
                    {
                        legatoList = std::move(list);
                        componentMap[name] = legatoList.get();
                    }
                }

                if (name == "notePriorityList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::NotePriority,
                                            Name::NotePriority, "menu-note-priority");
                        list != nullptr)
                    {
                        notePriorityList = std::move(list);
                        componentMap[name] = notePriorityList.get();
                    }
                }

                if (name == "polyphonyList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::Polyphony, Name::Polyphony,
                                            "menu-poly");
                        list != nullptr)
                    {
                        polyphonyList = std::move(list);
                        componentMap[name] = polyphonyList.get();
                    }
                }

                if (name == "unisonVoicesList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::UnisonVoices,
                                            Name::UnisonVoices, "menu-voices");
                        list != nullptr)
                    {
                        unisonVoicesList = std::move(list);
                        componentMap[name] = unisonVoicesList.get();
                    }
                }

                if (name == "bendUpRangeList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::PitchBendUpRange,
                                            Name::PitchBendUpRange, "menu-pitch-bend");
                        list != nullptr)
                    {
                        bendUpRangeList = std::move(list);
                        componentMap[name] = bendUpRangeList.get();
                    }
                }

                if (name == "bendDownRangeList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::PitchBendDownRange,
                                            Name::PitchBendDownRange, "menu-pitch-bend");
                        list != nullptr)
                    {
                        bendDownRangeList = std::move(list);
                        componentMap[name] = bendDownRangeList.get();
                    }
                }

                if (name == "menu")
                {
                    menuButton = addMenu(x, y, w, h, useAssetOrDefault(pic, "button-clear-red"));
                    componentMap[name] = menuButton.get();
                }

                if (name == "resonanceKnob")
                {
                    resonanceKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterResonance, 0.f,
                                Name::FilterResonance, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = resonanceKnob.get();
                }
                if (name == "cutoffKnob")
                {
                    cutoffKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterCutoff, 1.f,
                                         Name::FilterCutoff, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = cutoffKnob.get();
                }
                if (name == "filterEnvelopeAmtKnob")
                {
                    filterEnvelopeAmtKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterEnvAmount, 0.f,
                                Name::FilterEnvAmount, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = filterEnvelopeAmtKnob.get();
                }
                if (name == "multimodeKnob")
                {
                    multimodeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterMode, 0.f,
                                            Name::FilterMode, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = multimodeKnob.get();
                }

                if (name == "volumeKnob")
                {
                    volumeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Volume, 0.5f,
                                         Name::Volume, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = volumeKnob.get();
                }
                if (name == "portamentoKnob")
                {
                    portamentoKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Portamento, 0.f,
                                             Name::Portamento, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = portamentoKnob.get();
                }
                if (name == "osc1PitchKnob")
                {
                    osc1PitchKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc1Pitch, 0.5f,
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
                    componentMap[name] = osc1PitchKnob.get();
                }
                if (name == "pulseWidthKnob")
                {
                    pulseWidthKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::OscPulseWidth, 0.f,
                                             Name::OscPulseWidth, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = pulseWidthKnob.get();
                }
                if (name == "osc2PitchKnob")
                {
                    osc2PitchKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc2Pitch, 0.5f,
                                            Name::Osc2Pitch, useAssetOrDefault(pic, "knob"));
                    osc2PitchKnob->cmdDragCallback = [](const double value) {
                        const auto semitoneValue = (int)juce::jmap(value, -24.0, 24.0);
                        return juce::jmap((double)semitoneValue, -24.0, 24.0, 0.0, 1.0);
                    };
                    osc2PitchKnob->altDragCallback = [](const double value) {
                        const auto octValue = (int)juce::jmap(value, -2.0, 2.0);
                        return juce::jmap((double)octValue, -2.0, 2.0, 0.0, 1.0);
                    };
                    componentMap[name] = osc2PitchKnob.get();
                }

                if (name == "osc1MixKnob")
                {
                    osc1MixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc1Mix, 1.f,
                                          Name::Osc1Mix, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = osc1MixKnob.get();
                }
                if (name == "osc2MixKnob")
                {
                    osc2MixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc2Mix, 1.f,
                                          Name::Osc2Mix, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = osc2MixKnob.get();
                }
                if (name == "ringModMixKnob")
                {
                    ringModMixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::RingModMix, 0.f,
                                             Name::RingModMix, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = ringModMixKnob.get();
                }
                if (name == "noiseMixKnob")
                {
                    noiseMixKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::NoiseMix, 0.f,
                                           Name::NoiseMix, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = noiseMixKnob.get();
                }
                if (name == "noiseColorButton")
                {
                    noiseColorButton = addMultiStateButton(
                        x, y, w, h, ownerFilter, ID::NoiseColor, Name::NoiseColor,
                        useAssetOrDefault(pic, "button-slim-noise"), 3);
                    componentMap[name] = noiseColorButton.get();
                }

                if (name == "xmodKnob")
                {
                    xmodKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Crossmod, 0.f,
                                       Name::Crossmod, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = xmodKnob.get();
                }
                if (name == "osc2DetuneKnob")
                {
                    osc2DetuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc2Detune, 0.f,
                                             Name::Osc2Detune, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = osc2DetuneKnob.get();
                }

                if (name == "envPitchModKnob")
                {
                    envPitchModKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::EnvToPitch, 0.f,
                                              Name::EnvToPitch, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = envPitchModKnob.get();
                }
                if (name == "brightnessKnob")
                {
                    brightnessKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Brightness, 1.f,
                                             Name::Brightness, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = brightnessKnob.get();
                }

                if (name == "attackKnob")
                {
                    attackKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::AmpEnvAttack, 0.f,
                                         Name::AmpEnvAttack, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = attackKnob.get();
                }
                if (name == "decayKnob")
                {
                    decayKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::AmpEnvDecay, 0.f,
                                        Name::AmpEnvDecay, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = decayKnob.get();
                }
                if (name == "sustainKnob")
                {
                    sustainKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::AmpEnvSustain, 1.f,
                                          Name::AmpEnvSustain, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = sustainKnob.get();
                }
                if (name == "releaseKnob")
                {
                    releaseKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::AmpEnvRelease, 0.f,
                                          Name::AmpEnvRelease, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = releaseKnob.get();
                }

                if (name == "fattackKnob")
                {
                    fattackKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterEnvAttack, 0.f,
                                          Name::FilterEnvAttack, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = fattackKnob.get();
                }
                if (name == "fdecayKnob")
                {
                    fdecayKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterEnvDecay, 0.f,
                                         Name::FilterEnvDecay, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = fdecayKnob.get();
                }
                if (name == "fsustainKnob")
                {
                    fsustainKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterEnvSustain, 1.f,
                                Name::FilterEnvSustain, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = fsustainKnob.get();
                }
                if (name == "freleaseKnob")
                {
                    freleaseKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterEnvRelease, 0.f,
                                Name::FilterEnvRelease, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = freleaseKnob.get();
                }

                if (name == "lfoFrequencyKnob")
                {
                    lfoFrequencyKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::Lfo1Rate, 0.5f, Name::Lfo1Rate,
                                useAssetOrDefault(pic, "knob")); // 4 Hz
                    componentMap[name] = lfoFrequencyKnob.get();
                }
                if (name == "lfoAmt1Knob")
                {
                    lfoAmt1Knob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::LfoModAmt1, 0.f,
                                          Name::LfoModAmt1, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoAmt1Knob.get();
                }
                if (name == "lfoAmt2Knob")
                {
                    lfoAmt2Knob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::LfoModAmt2, 0.f,
                                          Name::LfoModAmt2, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoAmt2Knob.get();
                }

                if (name == "lfoWave1Knob")
                {
                    lfoWave1Knob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Lfo1Wave1, 0.5f,
                                           Name::Lfo1Wave1, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoWave1Knob.get();
                }
                if (name == "lfoWave2Knob")
                {
                    lfoWave2Knob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Lfo1Wave2, 0.5f,
                                           Name::Lfo1Wave2, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoWave2Knob.get();
                }
                if (name == "lfoWave3Knob")
                {
                    lfoWave3Knob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Lfo1Wave3, 0.5f,
                                           Name::Lfo1Wave3, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoWave3Knob.get();
                }

                if (name == "lfoPWSlider")
                {
                    lfoPWSlider = addKnob(x, y, w, h, d, fh, ownerFilter, ID::LfoPulseWidth, 0.f,
                                          Name::LfoPulseWidth, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = lfoPWSlider.get();
                }

                if (name == "lfoOsc1Button")
                {
                    lfoOsc1Button =
                        addButton(x, y, w, h, ownerFilter, ID::LfoToOsc1Pitch, Name::LfoToOsc1Pitch,
                                  useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoOsc1Button.get();
                }
                if (name == "lfoOsc2Button")
                {
                    lfoOsc2Button =
                        addButton(x, y, w, h, ownerFilter, ID::LfoToOsc2Pitch, Name::LfoToOsc2Pitch,
                                  useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoOsc2Button.get();
                }
                if (name == "lfoFilterButton")
                {
                    lfoFilterButton =
                        addButton(x, y, w, h, ownerFilter, ID::LfoToFilterCutoff,
                                  Name::LfoToFilterCutoff, useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoFilterButton.get();
                }

                if (name == "lfoPwm1Button")
                {
                    lfoPwm1Button = addButton(x, y, w, h, ownerFilter, ID::LfoToOsc1PW,
                                              Name::LfoToOsc1PW, useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoPwm1Button.get();
                }
                if (name == "lfoPwm2Button")
                {
                    lfoPwm2Button = addButton(x, y, w, h, ownerFilter, ID::LfoToOsc2PW,
                                              Name::LfoToOsc2PW, useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoPwm2Button.get();
                }
                if (name == "lfoVolumeButton")
                {
                    lfoVolumeButton =
                        addButton(x, y, w, h, ownerFilter, ID::LfoToVolume, Name::LfoToVolume,
                                  useAssetOrDefault(pic, "button"));
                    componentMap[name] = lfoVolumeButton.get();
                }

                if (name == "hardSyncButton")
                {
                    hardSyncButton = addButton(x, y, w, h, ownerFilter, ID::OscSync, Name::OscSync,
                                               useAssetOrDefault(pic, "button"));
                    componentMap[name] = hardSyncButton.get();
                }
                if (name == "osc1SawButton")
                {
                    osc1SawButton = addButton(x, y, w, h, ownerFilter, ID::Osc1SawWave,
                                              Name::Osc1SawWave, useAssetOrDefault(pic, "button"));
                    componentMap[name] = osc1SawButton.get();
                }
                if (name == "osc2SawButton")
                {
                    osc2SawButton = addButton(x, y, w, h, ownerFilter, ID::Osc2SawWave,
                                              Name::Osc2SawWave, useAssetOrDefault(pic, "button"));
                    componentMap[name] = osc2SawButton.get();
                }

                if (name == "osc1PulButton")
                {
                    osc1PulButton =
                        addButton(x, y, w, h, ownerFilter, ID::Osc1PulseWave, Name::Osc1PulseWave,
                                  useAssetOrDefault(pic, "button"));
                    componentMap[name] = osc1PulButton.get();
                }
                if (name == "osc2PulButton")
                {
                    osc2PulButton =
                        addButton(x, y, w, h, ownerFilter, ID::Osc2PulseWave, Name::Osc2PulseWave,
                                  useAssetOrDefault(pic, "button"));
                    componentMap[name] = osc2PulButton.get();
                }

                if (name == "pitchEnvInvertButton")
                {
                    pitchEnvInvertButton = addButton(x, y, w, h, ownerFilter, ID::EnvToPitchInvert,
                                                     Name::EnvToPitchInvert, "button-slim");
                    componentMap[name] = pitchEnvInvertButton.get();
                }

                if (name == "pwEnvInvertButton")
                {
                    pwEnvInvertButton = addButton(x, y, w, h, ownerFilter, ID::EnvToPWInvert,
                                                  Name::EnvToPWInvert, "button-slim");
                    componentMap[name] = pwEnvInvertButton.get();
                }

                if (name == "filterBPBlendButton")
                {
                    filterBPBlendButton = addButton(x, y, w, h, ownerFilter, ID::Filter2PoleBPBlend,
                                                    Name::Filter2PoleBPBlend, "button-slim");
                    componentMap[name] = filterBPBlendButton.get();
                }
                if (name == "fourPoleButton")
                {
                    fourPoleButton = addButton(x, y, w, h, ownerFilter, ID::Filter4PoleMode,
                                               Name::Filter4PoleMode, "button-slim");
                    componentMap[name] = fourPoleButton.get();
                }
                if (name == "oversamplingButton")
                {
                    oversamplingButton = addButton(x, y, w, h, ownerFilter, ID::HQMode,
                                                   Name::HQMode, useAssetOrDefault(pic, "button"));
                    componentMap[name] = oversamplingButton.get();
                }

                if (name == "filterKeyFollowKnob")
                {
                    filterKeyFollowKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterKeyFollow, 0.f,
                                Name::FilterKeyFollow, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = filterKeyFollowKnob.get();
                }

                if (name == "unisonButton")
                {
                    unisonButton = addButton(x, y, w, h, ownerFilter, ID::Unison, Name::Unison,
                                             useAssetOrDefault(pic, "button"));
                    componentMap[name] = unisonButton.get();
                }

                if (name == "tuneKnob")
                {
                    tuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Tune, 0.5f, Name::Tune,
                                       useAssetOrDefault(pic, "knob"));
                    componentMap[name] = tuneKnob.get();
                }
                if (name == "transposeKnob")
                {
                    transposeKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Transpose, 0.5f,
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

                    componentMap[name] = transposeKnob.get();
                }

                if (name == "voiceDetuneKnob")
                {
                    voiceDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::UnisonDetune, 0.25f,
                                Name::UnisonDetune, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = voiceDetuneKnob.get();
                }

                if (name == "vibratoWaveButton")
                {
                    vibratoWaveButton = addButton(x, y, w, h, ownerFilter, ID::VibratoWave,
                                                  Name::VibratoWave, "button-slim-vibrato-wave");
                    componentMap[name] = vibratoWaveButton.get();
                }
                if (name == "vibratoRateKnob")
                {
                    vibratoRateKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::VibratoRate, 0.3f,
                                Name::VibratoRate, useAssetOrDefault(pic, "knob")); // 5 Hz
                    componentMap[name] = vibratoRateKnob.get();
                }

                if (name == "veloFltEnvSlider")
                {
                    veloFltEnvSlider =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::VelToFilterEnv, 0.f,
                                Name::VelToFilterEnv, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = veloFltEnvSlider.get();
                }
                if (name == "veloAmpEnvSlider")
                {
                    veloAmpEnvSlider = addKnob(x, y, w, h, d, fh, ownerFilter, ID::VelToAmpEnv, 0.f,
                                               Name::VelToAmpEnv, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = veloAmpEnvSlider.get();
                }
                if (name == "midiLearnButton")
                {
                    midiLearnButton = addButton(x, y, w, h, ownerFilter, juce::String{},
                                                Name::MidiLearn, useAssetOrDefault(pic, "button"));
                    componentMap[name] = midiLearnButton.get();
                    midiLearnButton->onClick = [this]() {
                        const bool state = midiLearnButton->getToggleState();
                        paramManager.midiLearnAttachment.set(state);
                    };
                }
                if (name == "midiUnlearnButton")
                {
                    midiUnlearnButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::MidiUnlearn,
                                  useAssetOrDefault(pic, "button-clear"));
                    componentMap[name] = midiUnlearnButton.get();
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
                        auto paramId = fmt::format("PanVoice{}", which);
                        panKnobs[whichIdx] = addKnob(x, y, w, h, d, fh, ownerFilter, paramId, 0.5f,
                                                     fmt::format("Pan Voice {}", which),
                                                     useAssetOrDefault(pic, "knob"));
                        componentMap[name] = panKnobs[whichIdx].get();
                    }
                }

                if (name == "bendOsc2OnlyButton")
                {
                    bendOsc2OnlyButton =
                        addButton(x, y, w, h, ownerFilter, ID::PitchBendOsc2Only,
                                  Name::PitchBendOsc2Only, useAssetOrDefault(pic, "button"));
                    componentMap[name] = bendOsc2OnlyButton.get();
                }

                if (name == "filterDetuneKnob")
                {
                    filterDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::FilterSlop, 0.25f,
                                Name::FilterSlop, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = filterDetuneKnob.get();
                }
                if (name == "portamentoDetuneKnob")
                {
                    portamentoDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::PortamentoSlop, 0.25f,
                                Name::PortamentoSlop, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = portamentoDetuneKnob.get();
                }
                if (name == "envelopeDetuneKnob")
                {
                    envelopeDetuneKnob =
                        addKnob(x, y, w, h, d, fh, ownerFilter, ID::EnvelopeSlop, 0.25f,
                                Name::EnvelopeSlop, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = envelopeDetuneKnob.get();
                }
                if (name == "volumeDetuneKnob")
                {
                    volumeDetuneKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::LevelSlop, 0.25f,
                                               Name::LevelSlop, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = volumeDetuneKnob.get();
                }
                if (name == "lfoSyncButton")
                {
                    lfoSyncButton =
                        addButton(x, y, w, h, ownerFilter, ID::Lfo1TempoSync, Name::Lfo1TempoSync,
                                  useAssetOrDefault(pic, "button-slim"));
                    componentMap[name] = lfoSyncButton.get();
                }
                if (name == "pwEnvKnob")
                {
                    pwEnvKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::EnvToPW, 0.f,
                                        Name::EnvToPW, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = pwEnvKnob.get();
                }
                if (name == "pwEnvBothButton")
                {
                    pwEnvBothButton =
                        addButton(x, y, w, h, ownerFilter, ID::EnvToPWBothOscs,
                                  Name::EnvToPWBothOscs, useAssetOrDefault(pic, "button-slim"));
                    componentMap[name] = pwEnvBothButton.get();
                }
                if (name == "envPitchBothButton")
                {
                    envPitchBothButton =
                        addButton(x, y, w, h, ownerFilter, ID::EnvToPitchBothOscs,
                                  Name::EnvToPitchBothOscs, useAssetOrDefault(pic, "button-slim"));
                    componentMap[name] = envPitchBothButton.get();
                }
                if (name == "fenvInvertButton")
                {
                    fenvInvertButton =
                        addButton(x, y, w, h, ownerFilter, ID::FilterEnvInvert,
                                  Name::FilterEnvInvert, useAssetOrDefault(pic, "button-slim"));
                    componentMap[name] = fenvInvertButton.get();
                }
                if (name == "pwOffsetKnob")
                {
                    pwOffsetKnob = addKnob(x, y, w, h, d, fh, ownerFilter, ID::Osc2PWOffset, 0.f,
                                           Name::Osc2PWOffset, useAssetOrDefault(pic, "knob"));
                    componentMap[name] = pwOffsetKnob.get();
                }
                if (name == "selfOscPushButton")
                {
                    selfOscPushButton =
                        addButton(x, y, w, h, ownerFilter, ID::Filter2PolePush,
                                  Name::Filter2PolePush, useAssetOrDefault(pic, "button-slim"));
                    componentMap[name] = selfOscPushButton.get();
                }
                if (name == "xpanderFilterButton")
                {
                    xpanderFilterButton =
                        addButton(x, y, w, h, ownerFilter, ID::Filter4PoleXpander,
                                  Name::Filter4PoleXpander, useAssetOrDefault(pic, "button"));
                    componentMap[name] = xpanderFilterButton.get();
                }
                if (name == "xpanderModeList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, ID::FilterXpanderMode,
                                            Name::FilterXpanderMode, "menu-xpander");
                        list != nullptr)
                    {
                        xpanderModeList = std::move(list);
                        componentMap[name] = xpanderModeList.get();
                    }
                }

                if (name == "patchNumberList")
                {
                    if (auto list = addList(x, y, w, h, ownerFilter, juce::String{}, "Patch List",
                                            "menu-patch");
                        list != nullptr)
                    {
                        patchNumberList = std::move(list);
                        componentMap[name] = patchNumberList.get();

                        patchNumberList->onChange = [this]() {
                            processor.setCurrentProgram(patchNumberList->getSelectedId() - 1);
                        };
                    }
                }

                if (name == "prevPatchButton")
                {
                    prevPatchButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::PrevPatch,
                                  useAssetOrDefault(pic, "button-clear"));
                    componentMap[name] = prevPatchButton.get();
                    prevPatchButton->onClick = [this]() { prevProgram(); };
                }
                if (name == "nextPatchButton")
                {
                    nextPatchButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::NextPatch,
                                  useAssetOrDefault(pic, "button-clear"));
                    componentMap[name] = nextPatchButton.get();
                    nextPatchButton->onClick = [this]() { nextProgram(); };
                }

                if (name == "initPatchButton")
                {
                    initPatchButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::InitializePatch,
                                  useAssetOrDefault(pic, "button-clear-red"));
                    componentMap[name] = initPatchButton.get();
                    initPatchButton->onClick = [this]() {
                        MenuActionCallback(MenuAction::InitializePatch);
                    };
                }
                if (name == "randomizePatchButton")
                {
                    randomizePatchButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::RandomizePatch,
                                  useAssetOrDefault(pic, "button-clear-white"));
                    componentMap[name] = randomizePatchButton.get();
                    /*                     randomizePatchButton->onClick = [this]() {
                                            MenuActionCallback(MenuAction::RandomizePatch);
                                        }; */
                }

                if (name == "groupSelectButton")
                {
                    groupSelectButton =
                        addButton(x, y, w, h, ownerFilter, juce::String{}, Name::PatchGroupSelect,
                                  useAssetOrDefault(pic, "button-alt"));
                    componentMap[name] = groupSelectButton.get();
                }

                if (name.startsWith("select") && name.endsWith("Button"))
                {
                    auto which = name.retainCharacters("0123456789").getIntValue();
                    auto whichIdx = which - 1;

                    if (whichIdx >= 0 && whichIdx < NUM_PATCHES_PER_GROUP)
                    {
                        selectLabels[whichIdx] = addLabel(
                            x, y, w, h, h, fmt::format("Select Group/Patch {} Label", which),
                            "button-group-patch");
                        componentMap[name.replace("Button", "Label")] =
                            selectLabels[whichIdx].get();

                        selectButtons[whichIdx] =
                            addButton(x, y, w, h, ownerFilter, juce::String{},
                                      fmt::format("Select Group/Patch {}", which), "");

                        componentMap[name] = selectButtons[whichIdx].get();

                        selectButtons[whichIdx]->setTriggeredOnMouseDown(true);
                        selectButtons[whichIdx]->setClickingTogglesState(false);

                        selectButtons[whichIdx]->onClick = [this, whichIdx]() {
                            uint8_t curGroup = processor.getCurrentProgram() / 16;
                            uint8_t curPatchInGroup = processor.getCurrentProgram() % 16;

                            if (groupSelectButton->getToggleState())
                                curGroup = whichIdx;
                            else
                                curPatchInGroup = whichIdx;

                            processor.setCurrentProgram((curGroup * NUM_PATCHES_PER_GROUP) +
                                                        curPatchInGroup);
                        };

                        selectButtons[whichIdx]->onStateChange = [this]() {
                            updateSelectButtonStates();
                        };
                    }
                }

                if (name == "filterModeLabel")
                {
                    if (auto label =
                            addLabel(x, y, w, h, fh, "Filter Mode Label", "label-filter-mode");
                        label != nullptr)
                    {
                        filterModeLabel = std::move(label);
                        filterModeLabel->toBack();
                        componentMap[name] = filterModeLabel.get();
                    }
                }

                if (name == "filterOptionsLabel")
                {
                    if (auto label = addLabel(x, y, w, h, fh, "Filter Options Label",
                                              "label-filter-options");
                        label != nullptr)
                    {
                        filterOptionsLabel = std::move(label);
                        filterOptionsLabel->toBack();
                        componentMap[name] = filterOptionsLabel.get();
                    }
                }

                if (name == "lfoWave2Label")
                {
                    if (auto label =
                            addLabel(x, y, w, h, fh, "LFO Wave 2 Icons", "label-lfo-wave2");
                        label != nullptr)
                    {
                        lfoWave2Label = std::move(label);
                        lfoWave2Label->toBack();
                        componentMap[name] = lfoWave2Label.get();
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

        auto *param = paramManager.getParameterManager().getParameter(ID::Polyphony);
        if (param)
        {
            const auto polyOption = param->getValue();
            polyphonyList->setScrollWheelEnabled(true);
            polyphonyList->setValue(polyOption, juce::dontSendNotification);
        }
    }

    if (unisonVoicesList)
    {
        for (int i = 1; i <= MAX_PANNINGS; ++i)
        {
            unisonVoicesList->addChoice(juce::String(i));
        }

        if (auto *param = paramManager.getParameterManager().getParameter(ID::UnisonVoices))
        {
            const auto uniVoicesOption = param->getValue();
            unisonVoicesList->setScrollWheelEnabled(true);
            unisonVoicesList->setValue(uniVoicesOption, juce::dontSendNotification);
        }
    }

    if (legatoList)
    {
        legatoList->addChoice("Both Envelopes");
        legatoList->addChoice("Filter Envelope Only");
        legatoList->addChoice("Amplifier Envelope Only");
        legatoList->addChoice("Always Retrigger");
        if (auto *param = paramManager.getParameterManager().getParameter(ID::EnvLegatoMode))
        {
            const auto legatoOption = param->getValue();
            legatoList->setScrollWheelEnabled(true);
            legatoList->setValue(legatoOption, juce::dontSendNotification);
        }
    }

    if (notePriorityList)
    {
        notePriorityList->addChoice("Last");
        notePriorityList->addChoice("Low");
        notePriorityList->addChoice("High");

        if (auto *param = paramManager.getParameterManager().getParameter(ID::NotePriority))
        {
            const auto notePrioOption = param->getValue();
            notePriorityList->setScrollWheelEnabled(true);
            notePriorityList->setValue(notePrioOption, juce::dontSendNotification);
        }
    }

    if (bendUpRangeList)
    {
        auto *menu = bendUpRangeList->getRootMenu();
        constexpr uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendUpRangeList->addChoice(juce::String(i));
        }

        if (auto *param = paramManager.getParameterManager().getParameter(ID::PitchBendUpRange))
        {
            const auto bendUpOption = param->getValue();
            bendUpRangeList->setScrollWheelEnabled(true);
            bendUpRangeList->setValue(bendUpOption, juce::dontSendNotification);
        }
    }

    if (bendDownRangeList)
    {
        auto *menu = bendDownRangeList->getRootMenu();
        constexpr uint8_t NUM_COLUMNS = 4;

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if ((i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendDownRangeList->addChoice(juce::String(i));
        }

        if (auto *param = paramManager.getParameterManager().getParameter(ID::PitchBendDownRange))
        {
            const auto bendDownOption = param->getValue();
            bendDownRangeList->setScrollWheelEnabled(true);
            bendDownRangeList->setValue(bendDownOption, juce::dontSendNotification);
        }
    }

    if (xpanderModeList)
    {
        xpanderModeList->addChoice("LP4");
        xpanderModeList->addChoice("LP3");
        xpanderModeList->addChoice("LP2");
        xpanderModeList->addChoice("LP1");
        xpanderModeList->addChoice("HP3");
        xpanderModeList->addChoice("HP2");
        xpanderModeList->addChoice("HP1");
        xpanderModeList->addChoice("BP4");
        xpanderModeList->addChoice("BP2");
        xpanderModeList->addChoice("N2");
        xpanderModeList->addChoice("PH3");
        xpanderModeList->addChoice("HP2+LP1");
        xpanderModeList->addChoice("HP3+LP1");
        xpanderModeList->addChoice("N2+LP1");
        xpanderModeList->addChoice("PH3+LP1");

        if (auto *param = paramManager.getParameterManager().getParameter(ID::FilterXpanderMode))
        {
            const auto xpanderModeOption = param->getValue();
            xpanderModeList->setScrollWheelEnabled(true);
            xpanderModeList->setValue(xpanderModeOption, juce::dontSendNotification);
        }
    }

    if (patchNumberList)
    {
        auto *menu = patchNumberList->getRootMenu();

        createPatchList(*menu, 0);

        patchNumberList->setScrollWheelEnabled(true);
        patchNumberList->setSelectedId(getCurrentProgramIndex() + 1);
    }

    createMenu();

    for (auto &[paramId, paramValue] : parameterValues)
    {
        if (componentMap.find(paramId) != componentMap.end())
        {
            Component *comp = componentMap[paramId];

            if (auto *knob = dynamic_cast<Knob *>(comp))
            {
                knob->setValue(paramValue, juce::dontSendNotification);
            }
            else if (auto *multiState = dynamic_cast<MultiStateButton *>(comp))
            {
                multiState->setValue(paramValue, juce::dontSendNotification);
            }
            else if (auto *button = dynamic_cast<ToggleButton *>(comp))
            {
                button->setToggleState(paramValue > 0.5f, juce::dontSendNotification);
            }
        }
    }

    ownerFilter.addChangeListener(this);

    skinLoaded = true;

    scaleFactorChanged();
    repaint();
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
#if defined(DEBUG) || defined(_DEBUG)
    inspector.reset();
#endif

    idleTimer->stopTimer();
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    menuButton.reset();
    popupMenus.clear();
    componentMap.clear();

    juce::PopupMenu::dismissAllActiveMenus();
}

void ObxfAudioProcessorEditor::idle()
{
    if (!skinLoaded || !fourPoleButton || !xpanderFilterButton || !filterBPBlendButton ||
        !filterModeLabel || !filterOptionsLabel || !unisonButton || !unisonVoicesList ||
        !patchNumberList || !patchNameLabel || !lfoWave2Label)
    {
        return;
    }

    if (!voiceLEDs.empty())
    {
        for (int i = 0; i < juce::jmin(polyphonyList->getSelectedId(), MAX_VOICES); i++)
        {
            const auto state =
                juce::roundToInt(juce::jmin(processor.getVoiceStatus(i), 1.f) * 24.f);

            if (voiceLEDs[i] && state != voiceLEDs[i]->getCurrentFrame())
            {
                voiceLEDs[i]->setCurrentFrame(state);
            }
        }
    }

    if (lfoWave2Label && lfoWave2Label->isVisible())
    {
        const auto state = juce::roundToInt(lfoPWSlider->getValue() * 24.f);

        if (lfoWave2Label && state != lfoWave2Label->getCurrentFrame())
        {
            lfoWave2Label->setCurrentFrame(state);
        }
    }

    const auto fourPole = fourPoleButton->getToggleState();
    const auto xpanderMode = xpanderFilterButton->getToggleState();
    const auto bpBlend = filterBPBlendButton->getToggleState();

    const auto filterModeFrame = fourPole ? (xpanderMode ? 3 : 2) : (bpBlend ? 1 : 0);

    if (filterModeLabel && filterModeFrame != filterModeLabel->getCurrentFrame())
    {
        filterModeLabel->setCurrentFrame(filterModeFrame);
    }

    if (filterOptionsLabel && fourPole != filterOptionsLabel->getCurrentFrame())
    {
        filterOptionsLabel->setCurrentFrame(fourPole);
    }

    filterBPBlendButton->setVisible(!fourPole);
    selfOscPushButton->setVisible(!fourPole);

    xpanderFilterButton->setVisible(fourPole);

    multimodeKnob->setVisible(!(fourPole && xpanderMode));
    xpanderModeList->setVisible(fourPole && xpanderMode);

    if (!patchNumberList->isPopupActive() &&
        patchNumberList->getSelectedId() != processor.getCurrentProgram() + 1)
    {
        patchNumberList->setSelectedId(processor.getCurrentProgram() + 1);
    }

    if (unisonButton->getToggleState() && unisonVoicesList->getAlpha() < 1.f)
    {
        unisonVoicesList->setAlpha(1.f);
    }
    if (!unisonButton->getToggleState() && unisonVoicesList->getAlpha() == 1.f)
    {
        unisonVoicesList->setAlpha(0.5f);
    }

    updateSelectButtonStates();

    if (!patchNameLabel->isBeingEdited())
    {
        patchNameLabel->setText(processor.getProgramName(processor.getCurrentProgram()),
                                juce::dontSendNotification);
    }
}

void ObxfAudioProcessorEditor::scaleFactorChanged()
{
    backgroundImage = getScaledImageFromCache("background");
    resized();
}

std::unique_ptr<Label> ObxfAudioProcessorEditor::addLabel(const int x, const int y, const int w,
                                                          const int h, int fh,
                                                          const juce::String &name,
                                                          const juce::String &assetName)
{
    if (fh == 0 && h > 0)
    {
        fh = h;
    }

    auto *label = new Label(assetName, fh, &processor);

    label->setDrawableBounds(transformBounds(x, y, w, h));
    label->setName(name);

    addAndMakeVisible(label);

    return std::unique_ptr<Label>(label);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(int x, int y, int w, int h, int d, int fh,
                                                        ObxfAudioProcessor & /*filter*/,
                                                        const juce::String &paramId, float defval,
                                                        const juce::String &name,
                                                        const juce::String &assetName)
{
    int frameHeight = defKnobDiameter;
    if (d > 0)
        frameHeight = d;
    else if (fh > 0)
        frameHeight = fh;

    auto *knob = new Knob(assetName, frameHeight, &processor);

    if (auto *param = paramManager.getParameterManager().getParameter(paramId); param != nullptr)
    {
        knob->setParameter(param);
        knob->setValue(param->getValue());
        knobAttachments.add(new KnobAttachment(
            paramManager.getParameterManager(), param, *knob,
            [](Knob &k, float v) { k.setValue(v, juce::dontSendNotification); },
            [](const Knob &k) { return static_cast<float>(k.getValue()); }));
    }

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

    addAndMakeVisible(knob);

    return std::unique_ptr<Knob>(knob);
}

std::unique_ptr<ToggleButton>
ObxfAudioProcessorEditor::addButton(const int x, const int y, const int w, const int h,
                                    ObxfAudioProcessor & /*filter*/, const juce::String &paramId,
                                    const juce::String &name, const juce::String &assetName)
{
    auto *button = new ToggleButton(assetName, h, &processor);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramManager.getParameterManager().getParameter(paramId))
        {
            button->setToggleState(param->getValue() > 0.5f, juce::dontSendNotification);
            toggleAttachments.add(new ButtonAttachment(
                paramManager.getParameterManager(), param, *button,
                [](ToggleButton &b, float v) {
                    b.setToggleState(v > 0.5f, juce::dontSendNotification);
                },
                [](const ToggleButton &b) { return b.getToggleState() ? 1.0f : 0.0f; }));
        }
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

std::unique_ptr<MultiStateButton> ObxfAudioProcessorEditor::addMultiStateButton(
    const int x, const int y, const int w, const int h, ObxfAudioProcessor & /*filter*/,
    const juce::String &paramId, const juce::String &name, const juce::String &assetName,
    const uint8_t numStates)
{
    auto *button = new MultiStateButton(assetName, &processor, numStates);

    if (auto *param = paramManager.getParameterManager().getParameter(paramId); param != nullptr)
    {
        button->setValue(param->getValue());
        multiStateAttachments.add(new MultiStateAttachment(
            paramManager.getParameterManager(), param, *button,
            [](MultiStateButton &b, float v) { b.setValue(v, juce::dontSendNotification); },
            [](const MultiStateButton &b) { return static_cast<float>(b.getValue()); }));
    }

    button->setBounds(x, y, w, h);
    button->setTitle(name);

    addAndMakeVisible(button);

    return std::unique_ptr<MultiStateButton>(button);
}

std::unique_ptr<ButtonList>
ObxfAudioProcessorEditor::addList(const int x, const int y, const int w, const int h,
                                  ObxfAudioProcessor & /*filter*/, const juce::String &paramId,
                                  const juce::String &name, const juce::String &assetName)
{
#if JUCE_WINDOWS || JUCE_LINUX
    auto *bl = new ButtonList(assetName, h, &processor);
#else
    auto *bl = new ButtonList(assetName, h, &processor);
#endif

    if (!paramId.isEmpty())
    {
        if (auto *param = paramManager.getParameterManager().getParameter(paramId))
        {
            buttonListAttachments.add(new ButtonListAttachment(
                paramManager.getParameterManager(), param, *bl,
                [](ButtonList &l, float v) { l.setValue(v, juce::dontSendNotification); },
                [](const ButtonList &l) { return l.getValue(); }));
        }
    }

    bl->setBounds(x, y, w, h);
    bl->setTitle(name);
    addAndMakeVisible(bl);

    return std::unique_ptr<ButtonList>(bl);
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

void ObxfAudioProcessorEditor::clean() { this->removeAllChildren(); }

void ObxfAudioProcessorEditor::rebuildComponents(ObxfAudioProcessor &ownerFilter)
{
    themeFolder = utils.getCurrentThemeFolder();

    ownerFilter.removeChangeListener(this);

    setSize(1440, 450);

    ownerFilter.addChangeListener(this);
    repaint();
}

juce::PopupMenu ObxfAudioProcessorEditor::createPatchList(juce::PopupMenu &menu,
                                                          const int itemIdxStart)
{
    const uint8_t NUM_COLUMNS = 8;

    uint8_t sectionCount = 0;

    for (int i = 0; i < processor.getNumPrograms(); ++i)
    {
        if (i > 0 && i % (processor.getNumPrograms() / NUM_COLUMNS) == 0)
        {
            menu.addColumnBreak();
        }

        if (i % NUM_PATCHES_PER_GROUP == 0)
        {
            sectionCount++;
            menu.addSectionHeader(fmt::format("Group {:d}", sectionCount));
        }

        menu.addItem(i + itemIdxStart + 1,
                     juce::String{i + 1}.paddedLeft('0', 3) + ": " + processor.getProgramName(i),
                     true, i == processor.getCurrentProgram());
    }

    return menu;
}

void ObxfAudioProcessorEditor::createMenu()
{
    juce::MemoryBlock memoryBlock;
    memoryBlock.fromBase64Encoding(juce::SystemClipboard::getTextFromClipboard());
    bool enablePasteOption = utils.isMemoryBlockAPatch(memoryBlock);
    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    themes = utils.getThemeFiles();
    banks = utils.getBankFiles();

    {
        juce::PopupMenu fileMenu;

        fileMenu.addItem(static_cast<int>(MenuAction::InitializePatch), "Initialize Patch", true,
                         false);

        fileMenu.addSeparator();

        fileMenu.addItem(MenuAction::ImportPatch, "Import Patch...", true, false);
        fileMenu.addItem(MenuAction::ImportBank, "Import Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(MenuAction::ExportPatch, "Export Patch...", true, false);
        fileMenu.addItem(MenuAction::ExportBank, "Export Bank...", true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(MenuAction::CopyPatch, "Copy Patch", true, false);
        fileMenu.addItem(MenuAction::PastePatch, "Paste Patch", enablePasteOption, false);

        menu->addSubMenu("File", fileMenu);
    }

    {
        juce::PopupMenu patchesMenu;
        menu->addSubMenu("Patches", createPatchList(patchesMenu, presetStart));
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

    createMidi(midiStart, midiMenu);

    menu->addSubMenu("MIDI Mappings", midiMenu);

    {
        juce::PopupMenu themeMenu;
        for (int i = 0; i < themes.size(); ++i)
        {
            const juce::File theme = themes.getUnchecked(i);
            themeMenu.addItem(i + themeStart + 1, theme.getFileName(), true,
                              theme.getFileName() == themeFolder.getFileName());
        }

        menu->addSubMenu("Themes", themeMenu);
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
            if (result >= (themeStart + 1) && result <= (themeStart + themes.size()))
            {
                result -= 1;
                result -= themeStart;

                const juce::File newThemeFolder = themes.getUnchecked(result);
                utils.setCurrentThemeFolder(newThemeFolder.getFileName());

                clean();
                loadTheme(processor);
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
            else if (result >= midiStart)
            {
                if (const auto selected_idx = result - midiStart; selected_idx < midiFiles.size())
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
    if (action == MenuAction::InitializePatch)
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
        knobAttachment->updateToControl();
    }
    for (const auto buttonListAttachment : buttonListAttachments)
    {
        buttonListAttachment->updateToControl();
    }
    for (const auto triStateAttachment : multiStateAttachments)
    {
        triStateAttachment->updateToControl();
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