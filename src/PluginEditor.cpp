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

#include "components/ScalingImageCache.h"

#include "gui/AboutScreen.h"
#include "gui/FocusDebugger.h"
#include "gui/FocusOrder.h"

#include "sst/plugininfra/misc_platform.h"

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
    : AudioProcessorEditor(&p), processor(p), utils(p.getUtils()),
      paramAdapter(p.getParamAdapter()), imageCache(utils), midiStart(5000), sizeStart(4000),
      themeStart(1000), themes(utils.getThemeLocations())
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
    keyCommandHandler->setRefreshThemeCallback([this]() {
        clean();
        loadTheme(processor);
    });

    commandManager.registerAllCommandsForTarget(keyCommandHandler.get());
    commandManager.setFirstCommandTarget(keyCommandHandler.get());
    commandManager.getKeyMappings()->resetToDefaultMappings();
    getTopLevelComponent()->addKeyListener(commandManager.getKeyMappings());

    aboutScreen = std::make_unique<AboutScreen>(*this);
    addChildComponent(*aboutScreen);

    const auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Jersey20_ttf,
                                                                  BinaryData::Jersey20_ttfSize);
    patchNameFont = juce::FontOptions(typeface);

    themeLocation = utils.getCurrentThemeLocation();

    loadTheme(processor);

    if (backgroundIsSVG)
    {
        const auto &bgi = imageCache.getSVGDrawable("background");

        initialWidth = bgi->getWidth();
        initialHeight = bgi->getHeight();
    }
    else
    {
        initialWidth = backgroundImage.getWidth();
        initialHeight = backgroundImage.getHeight();
    }

    if (noThemesAvailable)
    {
        initialWidth = 800;
        initialHeight = 400;
    }

    setSize(initialWidth, initialHeight);

    initialBounds = getLocalBounds();

    setResizable(true, false);

    constrainer = std::make_unique<juce::ComponentBoundsConstrainer>();
    constrainer->setMinimumSize(initialWidth * scaleFactors[0], initialHeight * scaleFactors[0]);
    constrainer->setFixedAspectRatio(static_cast<double>(initialWidth) / initialHeight);
    setConstrainer(constrainer.get());

    updateFromHost();

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
    processor.uiState.editorAttached = true;

#if defined(DEBUG) || defined(_DEBUG)
    inspector = std::make_unique<melatonin::Inspector>(*this, false);
    inspector->setVisible(false);
#endif

    focusDebugger = std::make_unique<FocusDebugger>(*this);
    // focusDebugger->setDoFocusDebug(true);

    auto sf = utils.getDefaultZoomFactor();

    const int newWidth = juce::roundToInt(static_cast<float>(initialWidth) * sf);
    const int newHeight = juce::roundToInt(static_cast<float>(initialHeight) * sf);

    setSize(newWidth, newHeight);

    // Hammer that size into place before we even try to show
    resized();
    // including forcing the larger assets to load if needed
    for (auto &[n, c] : componentMap)
    {
        if (auto hcf = dynamic_cast<HasScaleFactor *>(c))
        {
            hcf->scaleFactorChanged();
        }
    }
    resizeOnNextIdle = 3;
}

void ObxfAudioProcessorEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    auto swr = utils.getUseSoftwareRenderer();

    if (swr)
    {
        if (auto peer = getPeer())
        {
            peer->setCurrentRenderingEngine(0); // 0 for software mode, 1 for Direct2D mode
        }
    }
#endif

    for (auto *p = getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (auto dw = dynamic_cast<juce::DocumentWindow *>(p))
        {
            if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
            {
                // reapply document background color from LookAndFeel.h
                dw->setColour(
                    juce::DocumentWindow::backgroundColourId,
                    juce::Component::findColour(juce::DocumentWindow::backgroundColourId));
            }
        }
    }

    if (isShowing() && isVisible())
    {
        resized();
    }
}

void ObxfAudioProcessorEditor::resized()
{
    scaleFactorChanged();

    themeLocation = utils.getCurrentThemeLocation();

    if (!cachedThemeXml)
        return;

    if (cachedThemeXml->getTagName() == "obxf-theme")
    {
        static FocusOrder focusOrder;

        for (const auto *child : cachedThemeXml->getChildWithTagNameIterator("widget"))
        {
            juce::String name = child->getStringAttribute("name");

            const auto x = child->getIntAttribute("x");
            const auto y = child->getIntAttribute("y");
            const auto w = child->getIntAttribute("w");
            const auto h = child->getIntAttribute("h");
            const auto d = child->getIntAttribute("d");

            if (componentMap[name] != nullptr)
            {
                auto addOrder = [this, name]() {
                    auto tfo = focusOrder.getOrder(name.toStdString());

                    if (tfo != 0 && componentMap[name]->getTitle().isNotEmpty())
                    {
                        componentMap[name]->setExplicitFocusOrder(tfo);
                        componentMap[name]->setWantsKeyboardFocus(true);
                    }
                };

                if (dynamic_cast<Label *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    if (name.startsWith("voice") && name.endsWith("LED"))
                    {
                        componentMap[name.replace("LED", "BG")]->setBounds(
                            transformBounds(x, y, w, h));
                    }
                    else
                    {
                        addOrder();
                    }
                }
                else if (dynamic_cast<Display *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    addOrder();
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
                        knob->setBounds(transformBounds(x, y, defKnobDiameter, defKnobDiameter));
                    }

                    knob->setPopupDisplayEnabled(true, true, knob->getParentComponent());
                    addOrder();
                }
                else if (dynamic_cast<ButtonList *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    addOrder();
                }
                else if (dynamic_cast<ImageMenu *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    addOrder();
                }
                else if (dynamic_cast<MultiStateButton *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    addOrder();
                }
                else if (dynamic_cast<ToggleButton *>(componentMap[name]))
                {
                    componentMap[name]->setBounds(transformBounds(x, y, w, h));
                    addOrder();

                    if (name.startsWith("select") && name.endsWith("Button"))
                    {
                        componentMap[name.replace("Button", "Label")]->setBounds(
                            transformBounds(x, y, w, h));
                    }
                }
            }
        }
    }

    if (aboutScreen)
        aboutScreen->setBounds(getBounds());
}

void ObxfAudioProcessorEditor::updateSelectButtonStates() const
{
    OBLOGONCE(rework, "Setting current group and patch to 0/0 for programmer buttons")
    const uint8_t curGroup = 0;
    const uint8_t curPatchInGroup = 0;

    for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
    {
        uint8_t offset = 0;

        if (selectButtons[i] && selectButtons[i]->isDown())
            offset += 1;

        if (i == curGroup)
            offset += 2;

        if (i == curPatchInGroup)
            offset += 4;

        if (selectLabels[i])
            selectLabels[i]->setCurrentFrame(offset);
    }
}

void ObxfAudioProcessorEditor::loadTheme(ObxfAudioProcessor &ownerFilter)
{
    skinLoaded = false;

    if (!loadThemeFilesAndCheck())
        return;

    OBLOG(themes, "Setting theme to " << themeLocation.file.getFullPathName()
                                      << " (type=" << (int)themeLocation.locationType << ")");

    const auto parameterValues = saveComponentParameterValues();

    clearAndResetComponents(ownerFilter);

    if (themeLocation.locationType == Utils::EMBEDDED)
    {
        auto xml = juce::String(BinaryData::theme_xml, BinaryData::theme_xmlSize);
        juce::XmlDocument themeXml(xml);
        cachedThemeXml = themeXml.getDocumentElement();
    }
    else
    {
        const juce::File theme = themeLocation.file.getChildFile("theme.xml");
        juce::XmlDocument themeXml(theme);
        cachedThemeXml = themeXml.getDocumentElement();
    }

    if (!cachedThemeXml)
    {
        jassertfalse;
        return;
    }

    if (cachedThemeXml->getTagName() == "obxf-theme")
    {
        imageCache.clearCache();
        if (themeLocation.locationType == Utils::EMBEDDED)
        {
            imageCache.embeddedMode = true;
            imageCache.skinDir = juce::File();
        }
        else
        {
            imageCache.embeddedMode = false;
            imageCache.skinDir = themeLocation.file;
        }

        createComponentsFromXml(cachedThemeXml.get());
    }

    setupMenus();
    restoreComponentParameterValues(parameterValues);
    finalizeThemeLoad(ownerFilter);
    resized();
}

bool ObxfAudioProcessorEditor::loadThemeFilesAndCheck()
{
    themes = utils.getThemeLocations();
    if (themes.empty())
    {
        noThemesAvailable = true;
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "No Themes Found",
            juce::String() +
                "No themes were found in the Theme folder. The editor cannot be displayed. " +
                "Factory folder location is '" + utils.getDocumentFolder().getFullPathName() +
                "'.");
        repaint();
        return false;
    }
    noThemesAvailable = false;
    return true;
}

std::map<juce::String, float> ObxfAudioProcessorEditor::saveComponentParameterValues()
{
    std::map<juce::String, float> parameterValues;
    for (auto &[paramId, component] : componentMap)
    {
        if (const auto *knob = dynamic_cast<Knob *>(component))
            parameterValues[paramId] = static_cast<float>(knob->getValue());
        else if (const auto *multiState = dynamic_cast<MultiStateButton *>(component))
            parameterValues[paramId] = static_cast<float>(multiState->getValue());
        else if (const auto *button = dynamic_cast<ToggleButton *>(component))
            parameterValues[paramId] = button->getToggleState() ? 1.0f : 0.0f;
    }
    return parameterValues;
}

void ObxfAudioProcessorEditor::clearAndResetComponents(ObxfAudioProcessor &ownerFilter)
{
    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    mainMenu.reset();
    popupMenus.clear();
    componentMap.clear();
    ownerFilter.removeChangeListener(this);
    themeLocation = utils.getCurrentThemeLocation();
    for (auto &controls : lfoControls)
        controls.clear();
}

bool ObxfAudioProcessorEditor::parseAndCreateComponentsFromTheme()
{
    const juce::File theme = themeLocation.file.getChildFile("theme.xml");
    if (!theme.existsAsFile())
    {
        addMenu(14, 25, 23, 35, "button-clear-red");
        rebuildComponents(processor);
        return false;
    }

    juce::XmlDocument themeXml(theme);
    if (const auto doc = themeXml.getDocumentElement(); !doc)
    {
        notLoadTheme = true;
        setSize(1440, 486);
        return false;
    }
    else
    {
        if (doc->getTagName() == "obxf-theme")
        {
            createComponentsFromXml(doc.get());
        }

        resized();
        return true;
    }
}

void ObxfAudioProcessorEditor::createComponentsFromXml(const juce::XmlElement *doc)
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
            patchNameLabel =
                std::make_unique<Display>("Patch Name", [this]() { return impliedScaleFactor(); });

            patchNameLabel->setBounds(transformBounds(x, y, w, h));
            patchNameLabel->setJustificationType(juce::Justification::centred);
            patchNameLabel->setMinimumHorizontalScale(1.f);
            patchNameLabel->setFont(patchNameFont.withHeight(18));

            patchNameLabel->setColour(juce::Label::textColourId, juce::Colours::red);
            patchNameLabel->setColour(juce::Label::textWhenEditingColourId, juce::Colours::red);
            patchNameLabel->setColour(juce::Label::outlineWhenEditingColourId,
                                      juce::Colours::transparentBlack);
            patchNameLabel->setColour(juce::TextEditor::textColourId, juce::Colours::red);
            patchNameLabel->setColour(juce::TextEditor::highlightedTextColourId,
                                      juce::Colours::red);
            patchNameLabel->setColour(juce::TextEditor::highlightColourId,
                                      juce::Colour(0x30FFFFFF));
            patchNameLabel->setColour(juce::CaretComponent::caretColourId, juce::Colours::red);

            patchNameLabel->setVisible(true);

            addChildComponent(*patchNameLabel);

            auto safeThis = SafePointer(this);
            patchNameLabel->onTextChange = [safeThis]() {
                if (!safeThis)
                    return;
                safeThis->processor.getActiveProgram().setName(safeThis->patchNameLabel->getText());
                safeThis->setupPatchNumberMenu();
            };

            componentMap[name] = patchNameLabel.get();
        }

        if (name.startsWith("voice") && name.endsWith("LED"))
        {
            auto which = name.retainCharacters("0123456789").getIntValue();
            auto whichIdx = which - 1;

            if (whichIdx >= 0 && whichIdx < MAX_VOICES)
            {
                if (auto label = addLabel(
                        x, y, w, h, h, fmt::format("Voice {} BG", which),
                        useAssetOrDefault(pic, fmt::format("label-led{}", which / MAX_PANNINGS)));
                    label != nullptr)
                {
                    voiceBGs[whichIdx] = std::move(label);
                    componentMap[name.replace("LED", "BG")] = voiceBGs[whichIdx].get();
                }

                if (auto label = addLabel(
                        x, y, w, h, h, fmt::format("Voice {} LED", which),
                        useAssetOrDefault(pic, fmt::format("label-led{}", which / MAX_PANNINGS)));
                    label != nullptr)
                {
                    voiceLEDs[whichIdx] = std::move(label);
                    voiceLEDs[whichIdx]->setCurrentFrame(1);
                    componentMap[name] = voiceLEDs[whichIdx].get();
                }
            }
        }

        if (name == "envLegatoModeMenu")
        {
            if (auto list =
                    addList(x, y, w, h, ID::EnvLegatoMode, Name::EnvLegatoMode, "menu-legato");
                list != nullptr)
            {
                envLegatoModeMenu = std::move(list);
                componentMap[name] = envLegatoModeMenu.get();
            }
        }

        if (name == "notePriorityMenu")
        {
            if (auto list =
                    addList(x, y, w, h, ID::NotePriority, Name::NotePriority, "menu-note-priority");
                list != nullptr)
            {
                notePriorityMenu = std::move(list);
                componentMap[name] = notePriorityMenu.get();
            }
        }

        if (name == "polyphonyMenu")
        {
            if (auto list = addList(x, y, w, h, ID::Polyphony, Name::Polyphony, "menu-poly");
                list != nullptr)
            {
                polyphonyMenu = std::move(list);
                componentMap[name] = polyphonyMenu.get();
            }
        }

        if (name == "unisonVoicesMenu")
        {
            if (auto list =
                    addList(x, y, w, h, ID::UnisonVoices, Name::UnisonVoices, "menu-voices");
                list != nullptr)
            {
                unisonVoicesMenu = std::move(list);
                componentMap[name] = unisonVoicesMenu.get();
            }
        }

        if (name == "bendUpRangeMenu")
        {
            if (auto list =
                    addList(x, y, w, h, ID::BendUpRange, Name::BendUpRange, "menu-pitch-bend");
                list != nullptr)
            {
                bendUpRangeMenu = std::move(list);
                componentMap[name] = bendUpRangeMenu.get();
            }
        }

        if (name == "bendDownRangeMenu")
        {
            if (auto list =
                    addList(x, y, w, h, ID::BendDownRange, Name::BendDownRange, "menu-pitch-bend");
                list != nullptr)
            {
                bendDownRangeMenu = std::move(list);
                componentMap[name] = bendDownRangeMenu.get();
            }
        }

        if (name == "mainMenu")
        {
            mainMenu = addMenu(x, y, w, h, useAssetOrDefault(pic, "button-clear-red"));
            mainMenu->setTitle("Main Menu");
            componentMap[name] = mainMenu.get();
            juce::Timer::callAfterDelay(30, [w = juce::Component::SafePointer(this)]() {
                if (w)
                    w->keyboardFocusMainMenu();
            });
        }

        if (name == "filterResonanceKnob")
        {
            filterResonanceKnob = addKnob(x, y, w, h, d, fh, ID::FilterResonance, 0.f,
                                          Name::FilterResonance, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterResonanceKnob.get();
        }
        if (name == "filterCutoffKnob")
        {
            filterCutoffKnob = addKnob(x, y, w, h, d, fh, ID::FilterCutoff, 1.f, Name::FilterCutoff,
                                       useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterCutoffKnob.get();
        }
        if (name == "filterEnvAmountKnob")
        {
            filterEnvAmountKnob = addKnob(x, y, w, h, d, fh, ID::FilterEnvAmount, 0.f,
                                          Name::FilterEnvAmount, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterEnvAmountKnob.get();
        }
        if (name == "filterModeKnob")
        {
            filterModeKnob = addKnob(x, y, w, h, d, fh, ID::FilterMode, 0.f, Name::FilterMode,
                                     useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterModeKnob.get();
        }

        if (name == "volumeKnob")
        {
            volumeKnob = addKnob(x, y, w, h, d, fh, ID::Volume, 0.5f, Name::Volume,
                                 useAssetOrDefault(pic, "knob"));
            componentMap[name] = volumeKnob.get();
        }
        if (name == "portamentoKnob")
        {
            portamentoKnob = addKnob(x, y, w, h, d, fh, ID::Portamento, 0.f, Name::Portamento,
                                     useAssetOrDefault(pic, "knob"));
            componentMap[name] = portamentoKnob.get();
        }
        if (name == "osc1PitchKnob")
        {
            osc1PitchKnob = addKnob(x, y, w, h, d, fh, ID::Osc1Pitch, 0.5f, Name::Osc1Pitch,
                                    useAssetOrDefault(pic, "knob"));
            osc1PitchKnob->cmdDragCallback = [](const double value) {
                const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
                return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0, 1.0);
            };
            osc1PitchKnob->altDragCallback = [](const double value) {
                const int values[13]{-24, -19, -17, -12, -7, -5, 0, 5, 7, 12, 17, 19, 24};
                const auto snapValue = static_cast<int>(juce::jmap(value, 0.0, 12.0));
                return juce::jmap(static_cast<double>(values[snapValue]), -24.0, 24.0, 0.0, 1.0);
            };
            componentMap[name] = osc1PitchKnob.get();
        }
        if (name == "oscPWKnob")
        {
            oscPWKnob = addKnob(x, y, w, h, d, fh, ID::OscPW, 0.f, Name::OscPW,
                                useAssetOrDefault(pic, "knob"));
            componentMap[name] = oscPWKnob.get();
        }
        if (name == "osc2PitchKnob")
        {
            osc2PitchKnob = addKnob(x, y, w, h, d, fh, ID::Osc2Pitch, 0.5f, Name::Osc2Pitch,
                                    useAssetOrDefault(pic, "knob"));
            osc2PitchKnob->cmdDragCallback = [](const double value) {
                const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
                return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0, 1.0);
            };
            osc2PitchKnob->altDragCallback = [](const double value) {
                const int values[13]{-24, -19, -17, -12, -7, -5, 0, 5, 7, 12, 17, 19, 24};
                const auto snapValue = static_cast<int>(juce::jmap(value, 0.0, 12.0));
                return juce::jmap(static_cast<double>(values[snapValue]), -24.0, 24.0, 0.0, 1.0);
            };
            componentMap[name] = osc2PitchKnob.get();
        }

        if (name == "osc1VolKnob")
        {
            osc1VolKnob = addKnob(x, y, w, h, d, fh, ID::Osc1Vol, 1.f, Name::Osc1Vol,
                                  useAssetOrDefault(pic, "knob"));
            componentMap[name] = osc1VolKnob.get();
        }
        if (name == "osc2VolKnob")
        {
            osc2VolKnob = addKnob(x, y, w, h, d, fh, ID::Osc2Vol, 1.f, Name::Osc2Vol,
                                  useAssetOrDefault(pic, "knob"));
            componentMap[name] = osc2VolKnob.get();
        }
        if (name == "ringModVolKnob")
        {
            ringModVolKnob = addKnob(x, y, w, h, d, fh, ID::RingModVol, 0.f, Name::RingModVol,
                                     useAssetOrDefault(pic, "knob"));
            componentMap[name] = ringModVolKnob.get();
        }
        if (name == "noiseVolKnob")
        {
            noiseVolKnob = addKnob(x, y, w, h, d, fh, ID::NoiseVol, 0.f, Name::NoiseVol,
                                   useAssetOrDefault(pic, "knob"));
            componentMap[name] = noiseVolKnob.get();
        }
        if (name == "noiseColorButton")
        {
            noiseColorButton = addMultiStateButton(x, y, w, h, ID::NoiseColor, Name::NoiseColor,
                                                   useAssetOrDefault(pic, "button-slim-noise"), 3);
            componentMap[name] = noiseColorButton.get();
        }

        if (name == "oscCrossmodKnob")
        {
            oscCrossmodKnob = addKnob(x, y, w, h, d, fh, ID::OscCrossmod, 0.f, Name::OscCrossmod,
                                      useAssetOrDefault(pic, "knob"));
            componentMap[name] = oscCrossmodKnob.get();
        }
        if (name == "osc2DetuneKnob")
        {
            osc2DetuneKnob = addKnob(x, y, w, h, d, fh, ID::Osc2Detune, 0.f, Name::Osc2Detune,
                                     useAssetOrDefault(pic, "knob"));
            componentMap[name] = osc2DetuneKnob.get();
        }

        if (name == "envToPitchAmountKnob")
        {
            envToPitchAmountKnob = addKnob(x, y, w, h, d, fh, ID::EnvToPitchAmount, 0.f,
                                           Name::EnvToPitchAmount, useAssetOrDefault(pic, "knob"));
            envToPitchAmountKnob->cmdDragCallback = [](const double value) {
                const auto semitoneValue = static_cast<int>(juce::jmap(value, 0.0, 36.0));
                return juce::jmap(static_cast<double>(semitoneValue), 0.0, 36.0, 0.0, 1.0);
            };
            envToPitchAmountKnob->altDragCallback = [](const double value) {
                const int values[10]{0, 5, 7, 12, 17, 19, 24, 29, 31, 36};
                const auto snapValue = static_cast<int>(juce::jmap(value, 0.0, 9.0));
                return juce::jmap(static_cast<double>(values[snapValue]), 0.0, 36.0, 0.0, 1.0);
            };
            componentMap[name] = envToPitchAmountKnob.get();
        }
        if (name == "oscBrightnessKnob")
        {
            oscBrightnessKnob = addKnob(x, y, w, h, d, fh, ID::OscBrightness, 1.f,
                                        Name::OscBrightness, useAssetOrDefault(pic, "knob"));
            componentMap[name] = oscBrightnessKnob.get();
        }

        if (name == "ampEnvAttackKnob")
        {
            ampEnvAttackKnob = addKnob(x, y, w, h, d, fh, ID::AmpEnvAttack, 0.f, Name::AmpEnvAttack,
                                       useAssetOrDefault(pic, "knob"));
            componentMap[name] = ampEnvAttackKnob.get();
        }
        if (name == "ampEnvDecayKnob")
        {
            ampEnvDecayKnob = addKnob(x, y, w, h, d, fh, ID::AmpEnvDecay, 0.f, Name::AmpEnvDecay,
                                      useAssetOrDefault(pic, "knob"));
            componentMap[name] = ampEnvDecayKnob.get();
        }
        if (name == "ampEnvSustainKnob")
        {
            ampEnvSustainKnob = addKnob(x, y, w, h, d, fh, ID::AmpEnvSustain, 1.f,
                                        Name::AmpEnvSustain, useAssetOrDefault(pic, "knob"));
            componentMap[name] = ampEnvSustainKnob.get();
        }
        if (name == "ampEnvReleaseKnob")
        {
            ampEnvReleaseKnob = addKnob(x, y, w, h, d, fh, ID::AmpEnvRelease, 0.f,
                                        Name::AmpEnvRelease, useAssetOrDefault(pic, "knob"));
            componentMap[name] = ampEnvReleaseKnob.get();
        }

        if (name == "filterEnvAttackKnob")
        {
            filterEnvAttackKnob = addKnob(x, y, w, h, d, fh, ID::FilterEnvAttack, 0.f,
                                          Name::FilterEnvAttack, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterEnvAttackKnob.get();
        }
        if (name == "filterEnvDecayKnob")
        {
            filterEnvDecayKnob = addKnob(x, y, w, h, d, fh, ID::FilterEnvDecay, 0.f,
                                         Name::FilterEnvDecay, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterEnvDecayKnob.get();
        }
        if (name == "filterEnvSustainKnob")
        {
            filterEnvSustainKnob = addKnob(x, y, w, h, d, fh, ID::FilterEnvSustain, 1.f,
                                           Name::FilterEnvSustain, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterEnvSustainKnob.get();
        }
        if (name == "filterEnvReleaseKnob")
        {
            filterEnvReleaseKnob = addKnob(x, y, w, h, d, fh, ID::FilterEnvRelease, 0.f,
                                           Name::FilterEnvRelease, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterEnvReleaseKnob.get();
        }

        if (name == "lfo1TempoSyncButton")
        {
            lfo1TempoSyncButton = addButton(x, y, w, h, ID::LFO1TempoSync, Name::LFO1TempoSync,
                                            useAssetOrDefault(pic, "button-slim"));
            componentMap[name] = lfo1TempoSyncButton.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1TempoSyncButton.get()));
        }
        if (name == "lfo1RateKnob")
        {
            lfo1RateKnob = addKnob(x, y, w, h, d, fh, ID::LFO1Rate, 0.5f, Name::LFO1Rate,
                                   useAssetOrDefault(pic, "knob")); // 4 Hz
            componentMap[name] = lfo1RateKnob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1RateKnob.get()));
        }
        if (name == "lfo1ModAmount1Knob")
        {
            lfo1ModAmount1Knob = addKnob(x, y, w, h, d, fh, ID::LFO1ModAmount1, 0.f,
                                         Name::LFO1ModAmount1, useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1ModAmount1Knob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1ModAmount1Knob.get()));
        }
        if (name == "lfo1ModAmount2Knob")
        {
            lfo1ModAmount2Knob = addKnob(x, y, w, h, d, fh, ID::LFO1ModAmount2, 0.f,
                                         Name::LFO1ModAmount2, useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1ModAmount2Knob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1ModAmount2Knob.get()));
        }
        if (name == "lfo1Wave1Knob")
        {
            lfo1Wave1Knob = addKnob(x, y, w, h, d, fh, ID::LFO1Wave1, 0.5f, Name::LFO1Wave1,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1Wave1Knob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1Wave1Knob.get()));
        }
        if (name == "lfo1Wave2Knob")
        {
            lfo1Wave2Knob = addKnob(x, y, w, h, d, fh, ID::LFO1Wave2, 0.5f, Name::LFO1Wave2,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1Wave2Knob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1Wave2Knob.get()));
        }
        if (name == "lfo1Wave3Knob")
        {
            lfo1Wave3Knob = addKnob(x, y, w, h, d, fh, ID::LFO1Wave3, 0.5f, Name::LFO1Wave3,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1Wave3Knob.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1Wave3Knob.get()));
        }
        if (name == "lfo1PWSlider")
        {
            lfo1PWSlider = addKnob(x, y, w, h, d, fh, ID::LFO1PW, 0.f, Name::LFO1PW,
                                   useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo1PWSlider.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1PWSlider.get()));
        }
        if (name == "lfo1ToOsc1PitchButton")
        {
            lfo1ToOsc1PitchButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToOsc1Pitch, Name::LFO1ToOsc1Pitch,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToOsc1PitchButton.get();
            lfoControls[0].emplace_back(
                dynamic_cast<juce::Component *>(lfo1ToOsc1PitchButton.get()));
        }
        if (name == "lfo1ToOsc2PitchButton")
        {
            lfo1ToOsc2PitchButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToOsc2Pitch, Name::LFO1ToOsc2Pitch,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToOsc2PitchButton.get();
            lfoControls[0].emplace_back(
                dynamic_cast<juce::Component *>(lfo1ToOsc2PitchButton.get()));
        }
        if (name == "lfo1ToFilterCutoffButton")
        {
            lfo1ToFilterCutoffButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToFilterCutoff, Name::LFO1ToFilterCutoff,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToFilterCutoffButton.get();
            lfoControls[0].emplace_back(
                dynamic_cast<juce::Component *>(lfo1ToFilterCutoffButton.get()));
        }
        if (name == "lfo1ToOsc1PWButton")
        {
            lfo1ToOsc1PWButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToOsc1PW, Name::LFO1ToOsc1PW,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToOsc1PWButton.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1ToOsc1PWButton.get()));
        }
        if (name == "lfo1ToOsc2PWButton")
        {
            lfo1ToOsc2PWButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToOsc2PW, Name::LFO1ToOsc2PW,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToOsc2PWButton.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1ToOsc2PWButton.get()));
        }
        if (name == "lfo1ToVolumeButton")
        {
            lfo1ToVolumeButton =
                addMultiStateButton(x, y, w, h, ID::LFO1ToVolume, Name::LFO1ToVolume,
                                    useAssetOrDefault(pic, "button-dual"), 3);
            componentMap[name] = lfo1ToVolumeButton.get();
            lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1ToVolumeButton.get()));
        }
        if (name == "lfo1Wave2Label")
        {
            if (auto label = addLabel(x, y, w, h, h, "LFO 1 Wave 2 Icons", "label-lfo-wave2");
                label != nullptr)
            {
                lfo1Wave2Label = std::move(label);
                lfo1Wave2Label->toBack();
                componentMap[name] = lfo1Wave2Label.get();
                lfoControls[0].emplace_back(dynamic_cast<juce::Component *>(lfo1Wave2Label.get()));
            }
        }

        if (name == "lfo2TempoSyncButton")
        {
            lfo2TempoSyncButton = addButton(x, y, w, h, ID::LFO2TempoSync, Name::LFO2TempoSync,
                                            useAssetOrDefault(pic, "button-slim-alt"));
            componentMap[name] = lfo2TempoSyncButton.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2TempoSyncButton.get()));
        }
        if (name == "lfo2RateKnob")
        {
            lfo2RateKnob = addKnob(x, y, w, h, d, fh, ID::LFO2Rate, 0.5f, Name::LFO2Rate,
                                   useAssetOrDefault(pic, "knob")); // 4 Hz
            componentMap[name] = lfo2RateKnob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2RateKnob.get()));
        }
        if (name == "lfo2ModAmount1Knob")
        {
            lfo2ModAmount1Knob = addKnob(x, y, w, h, d, fh, ID::LFO2ModAmount1, 0.f,
                                         Name::LFO2ModAmount1, useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2ModAmount1Knob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2ModAmount1Knob.get()));
        }
        if (name == "lfo2ModAmount2Knob")
        {
            lfo2ModAmount2Knob = addKnob(x, y, w, h, d, fh, ID::LFO2ModAmount2, 0.f,
                                         Name::LFO2ModAmount2, useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2ModAmount2Knob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2ModAmount2Knob.get()));
        }
        if (name == "lfo2Wave1Knob")
        {
            lfo2Wave1Knob = addKnob(x, y, w, h, d, fh, ID::LFO2Wave1, 0.5f, Name::LFO2Wave1,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2Wave1Knob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2Wave1Knob.get()));
        }
        if (name == "lfo2Wave2Knob")
        {
            lfo2Wave2Knob = addKnob(x, y, w, h, d, fh, ID::LFO2Wave2, 0.5f, Name::LFO2Wave2,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2Wave2Knob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2Wave2Knob.get()));
        }
        if (name == "lfo2Wave3Knob")
        {
            lfo2Wave3Knob = addKnob(x, y, w, h, d, fh, ID::LFO2Wave3, 0.5f, Name::LFO2Wave3,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2Wave3Knob.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2Wave3Knob.get()));
        }
        if (name == "lfo2PWSlider")
        {
            lfo2PWSlider = addKnob(x, y, w, h, d, fh, ID::LFO2PW, 0.f, Name::LFO2PW,
                                   useAssetOrDefault(pic, "knob"));
            componentMap[name] = lfo2PWSlider.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2PWSlider.get()));
        }
        if (name == "lfo2ToOsc1PitchButton")
        {
            lfo2ToOsc1PitchButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToOsc1Pitch, Name::LFO2ToOsc1Pitch,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToOsc1PitchButton.get();
            lfoControls[1].emplace_back(
                dynamic_cast<juce::Component *>(lfo2ToOsc1PitchButton.get()));
        }
        if (name == "lfo2ToOsc2PitchButton")
        {
            lfo2ToOsc2PitchButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToOsc2Pitch, Name::LFO2ToOsc2Pitch,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToOsc2PitchButton.get();
            lfoControls[1].emplace_back(
                dynamic_cast<juce::Component *>(lfo2ToOsc2PitchButton.get()));
        }
        if (name == "lfo2ToFilterCutoffButton")
        {
            lfo2ToFilterCutoffButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToFilterCutoff, Name::LFO2ToFilterCutoff,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToFilterCutoffButton.get();
            lfoControls[1].emplace_back(
                dynamic_cast<juce::Component *>(lfo2ToFilterCutoffButton.get()));
        }
        if (name == "lfo2ToOsc1PWButton")
        {
            lfo2ToOsc1PWButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToOsc1PW, Name::LFO2ToOsc1PW,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToOsc1PWButton.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2ToOsc1PWButton.get()));
        }
        if (name == "lfo2ToOsc2PWButton")
        {
            lfo2ToOsc2PWButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToOsc2PW, Name::LFO2ToOsc2PW,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToOsc2PWButton.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2ToOsc2PWButton.get()));
        }
        if (name == "lfo2ToVolumeButton")
        {
            lfo2ToVolumeButton =
                addMultiStateButton(x, y, w, h, ID::LFO2ToVolume, Name::LFO2ToVolume,
                                    useAssetOrDefault(pic, "button-dual-alt"), 3);
            componentMap[name] = lfo2ToVolumeButton.get();
            lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2ToVolumeButton.get()));
        }
        if (name == "lfo2Wave2Label")
        {
            if (auto label = addLabel(x, y, w, h, h, "LFO 2 Wave 2 Icons", "label-lfo-wave2");
                label != nullptr)
            {
                lfo2Wave2Label = std::move(label);
                lfo2Wave2Label->toBack();
                componentMap[name] = lfo2Wave2Label.get();
                lfoControls[1].emplace_back(dynamic_cast<juce::Component *>(lfo2Wave2Label.get()));
            }
        }

        if (name == "oscSyncButton")
        {
            oscSyncButton =
                addButton(x, y, w, h, ID::OscSync, Name::OscSync, useAssetOrDefault(pic, "button"));
            componentMap[name] = oscSyncButton.get();
        }
        if (name == "osc1SawButton")
        {
            osc1SawButton = addButton(x, y, w, h, ID::Osc1SawWave, Name::Osc1SawWave,
                                      useAssetOrDefault(pic, "button"));
            componentMap[name] = osc1SawButton.get();
        }
        if (name == "osc2SawButton")
        {
            osc2SawButton = addButton(x, y, w, h, ID::Osc2SawWave, Name::Osc2SawWave,
                                      useAssetOrDefault(pic, "button"));
            componentMap[name] = osc2SawButton.get();
        }

        if (name == "osc1PulseButton")
        {
            osc1PulseButton = addButton(x, y, w, h, ID::Osc1PulseWave, Name::Osc1PulseWave,
                                        useAssetOrDefault(pic, "button"));
            componentMap[name] = osc1PulseButton.get();
        }
        if (name == "osc2PulseButton")
        {
            osc2PulseButton = addButton(x, y, w, h, ID::Osc2PulseWave, Name::Osc2PulseWave,
                                        useAssetOrDefault(pic, "button"));
            componentMap[name] = osc2PulseButton.get();
        }

        if (name == "osc1TriangleLabel")
        {
            if (auto label = addLabel(x, y, w, h, h, "Osc 1 Triangle Icon", "label-osc-triangle");
                label != nullptr)
            {
                osc1TriangleLabel = std::move(label);
                componentMap[name] = osc1TriangleLabel.get();
            }
        }

        if (name == "osc1PulseLabel")
        {
            if (auto label = addLabel(x, y, w, h, h, "Osc 1 Pulse Icon", "label-osc-pulse");
                label != nullptr)
            {
                osc1PulseLabel = std::move(label);
                componentMap[name] = osc1PulseLabel.get();
            }
        }

        if (name == "osc2TriangleLabel")
        {
            if (auto label = addLabel(x, y, w, h, h, "Osc 2 Triangle Icon", "label-osc-triangle");
                label != nullptr)
            {
                osc2TriangleLabel = std::move(label);
                componentMap[name] = osc2TriangleLabel.get();
            }
        }

        if (name == "osc2PulseLabel")
        {
            if (auto label = addLabel(x, y, w, h, h, "Osc 2 Pulse Icon", "label-osc-pulse");
                label != nullptr)
            {
                osc2PulseLabel = std::move(label);
                componentMap[name] = osc2PulseLabel.get();
            }
        }

        if (name == "envToPitchInvertButton")
        {
            envToPitchInvertButton =
                addButton(x, y, w, h, ID::EnvToPitchInvert, Name::EnvToPitchInvert, "button-slim");
            componentMap[name] = envToPitchInvertButton.get();
        }

        if (name == "envToPWInvertButton")
        {
            envToPWInvertButton =
                addButton(x, y, w, h, ID::EnvToPWInvert, Name::EnvToPWInvert, "button-slim");
            componentMap[name] = envToPWInvertButton.get();
        }

        if (name == "filter2PoleBPBlendButton")
        {
            filter2PoleBPBlendButton = addButton(x, y, w, h, ID::Filter2PoleBPBlend,
                                                 Name::Filter2PoleBPBlend, "button-slim");
            componentMap[name] = filter2PoleBPBlendButton.get();
        }
        if (name == "filter4PoleModeButton")
        {
            filter4PoleModeButton =
                addButton(x, y, w, h, ID::Filter4PoleMode, Name::Filter4PoleMode, "button-slim");
            componentMap[name] = filter4PoleModeButton.get();
        }
        if (name == "hqModeButton")
        {
            hqModeButton =
                addButton(x, y, w, h, ID::HQMode, Name::HQMode, useAssetOrDefault(pic, "button"));
            componentMap[name] = hqModeButton.get();
        }

        if (name == "filterKeyTrackKnob")
        {
            filterKeyTrackKnob = addKnob(x, y, w, h, d, fh, ID::FilterKeyTrack, 0.f,
                                         Name::FilterKeyTrack, useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterKeyTrackKnob.get();
        }

        if (name == "unisonButton")
        {
            unisonButton =
                addButton(x, y, w, h, ID::Unison, Name::Unison, useAssetOrDefault(pic, "button"));
            componentMap[name] = unisonButton.get();
        }

        if (name == "tuneKnob")
        {
            tuneKnob = addKnob(x, y, w, h, d, fh, ID::Tune, 0.5f, Name::Tune,
                               useAssetOrDefault(pic, "knob"));
            componentMap[name] = tuneKnob.get();
        }
        if (name == "transposeKnob")
        {
            transposeKnob = addKnob(x, y, w, h, d, fh, ID::Transpose, 0.5f, Name::Transpose,
                                    useAssetOrDefault(pic, "knob"));
            transposeKnob->altDragCallback = [](const double value) {
                const auto octValue = static_cast<int>(juce::jmap(value, -2.0, 2.0));
                return juce::jmap(static_cast<double>(octValue), -2.0, 2.0, 0.0, 1.0);
            };

            componentMap[name] = transposeKnob.get();
        }

        if (name == "unisonDetuneKnob")
        {
            unisonDetuneKnob = addKnob(x, y, w, h, d, fh, ID::UnisonDetune, 0.25f,
                                       Name::UnisonDetune, useAssetOrDefault(pic, "knob"));
            componentMap[name] = unisonDetuneKnob.get();
        }

        if (name == "vibratoWaveButton")
        {
            vibratoWaveButton = addButton(x, y, w, h, ID::VibratoWave, Name::VibratoWave,
                                          "button-slim-vibrato-wave");
            componentMap[name] = vibratoWaveButton.get();
        }
        if (name == "vibratoRateKnob")
        {
            vibratoRateKnob = addKnob(x, y, w, h, d, fh, ID::VibratoRate, 0.3f, Name::VibratoRate,
                                      useAssetOrDefault(pic, "knob")); // 5 Hz
            componentMap[name] = vibratoRateKnob.get();
        }

        if (name == "filterEnvAttackCurveSlider")
        {
            filterEnvAttackCurveSlider =
                addKnob(x, y, w, h, d, fh, ID::FilterEnvAttackCurve, 0.f,
                        Name::FilterEnvAttackCurve, useAssetOrDefault(pic, "slider-h"));
            componentMap[name] = filterEnvAttackCurveSlider.get();
        }

        if (name == "velToFilterEnvSlider")
        {
            velToFilterEnvSlider =
                addKnob(x, y, w, h, d, fh, ID::VelToFilterEnv, 0.f, Name::VelToFilterEnv,
                        useAssetOrDefault(pic, "slider-h"));
            componentMap[name] = velToFilterEnvSlider.get();
        }

        if (name == "ampEnvAttackCurveSlider")
        {
            ampEnvAttackCurveSlider =
                addKnob(x, y, w, h, d, fh, ID::AmpEnvAttackCurve, 0.f, Name::AmpEnvAttackCurve,
                        useAssetOrDefault(pic, "slider-h"));
            componentMap[name] = ampEnvAttackCurveSlider.get();
        }

        if (name == "velToAmpEnvSlider")
        {
            velToAmpEnvSlider = addKnob(x, y, w, h, d, fh, ID::VelToAmpEnv, 0.f, Name::VelToAmpEnv,
                                        useAssetOrDefault(pic, "slider-h"));
            componentMap[name] = velToAmpEnvSlider.get();
        }
        if (name == "midiLearnButton")
        {
            midiLearnButton = addButton(x, y, w, h, juce::String{}, Name::MidiLearn,
                                        useAssetOrDefault(pic, "button"));
            componentMap[name] = midiLearnButton.get();
            auto safeThis = SafePointer(this);
            midiLearnButton->onClick = [safeThis]() {
                if (!safeThis)
                    return;
                const bool state = safeThis->midiLearnButton->getToggleState();
                safeThis->paramAdapter.midiLearnAttachment.set(state);
            };
        }

        if (name.startsWith("pan") && name.endsWith("Knob"))
        {
            auto which = name.retainCharacters("12345678").getIntValue();

            if (auto whichIdx = which - 1; whichIdx >= 0 && whichIdx < MAX_PANNINGS)
            {
                auto paramId = fmt::format("PanVoice{}", which);
                panKnobs[whichIdx] =
                    addKnob(x, y, w, h, d, fh, paramId, 0.5f, fmt::format("Pan Voice {}", which),
                            useAssetOrDefault(pic, "knob"));
                componentMap[name] = panKnobs[whichIdx].get();
            }
        }

        if (name == "bendOsc2OnlyButton")
        {
            bendOsc2OnlyButton = addButton(x, y, w, h, ID::BendOsc2Only, Name::BendOsc2Only,
                                           useAssetOrDefault(pic, "button"));
            componentMap[name] = bendOsc2OnlyButton.get();
        }

        if (name == "filterSlopKnob")
        {
            filterSlopKnob = addKnob(x, y, w, h, d, fh, ID::FilterSlop, 0.25f, Name::FilterSlop,
                                     useAssetOrDefault(pic, "knob"));
            componentMap[name] = filterSlopKnob.get();
        }
        if (name == "portamentoSlopKnob")
        {
            portamentoSlopKnob = addKnob(x, y, w, h, d, fh, ID::PortamentoSlop, 0.25f,
                                         Name::PortamentoSlop, useAssetOrDefault(pic, "knob"));
            componentMap[name] = portamentoSlopKnob.get();
        }
        if (name == "envelopeSlopKnob")
        {
            envelopeSlopKnob = addKnob(x, y, w, h, d, fh, ID::EnvelopeSlop, 0.25f,
                                       Name::EnvelopeSlop, useAssetOrDefault(pic, "knob"));
            componentMap[name] = envelopeSlopKnob.get();
        }
        if (name == "levelSlopKnob")
        {
            levelSlopKnob = addKnob(x, y, w, h, d, fh, ID::LevelSlop, 0.25f, Name::LevelSlop,
                                    useAssetOrDefault(pic, "knob"));
            componentMap[name] = levelSlopKnob.get();
        }
        if (name == "envToPWAmountKnob")
        {
            envToPWAmountKnob = addKnob(x, y, w, h, d, fh, ID::EnvToPWAmount, 0.f,
                                        Name::EnvToPWAmount, useAssetOrDefault(pic, "knob"));
            componentMap[name] = envToPWAmountKnob.get();
        }
        if (name == "envToPWBothOscsButton")
        {
            envToPWBothOscsButton =
                addButton(x, y, w, h, ID::EnvToPWBothOscs, Name::EnvToPWBothOscs,
                          useAssetOrDefault(pic, "button-slim"));
            componentMap[name] = envToPWBothOscsButton.get();
        }
        if (name == "envToPitchBothOscsButton")
        {
            envToPitchBothOscsButton =
                addButton(x, y, w, h, ID::EnvToPitchBothOscs, Name::EnvToPitchBothOscs,
                          useAssetOrDefault(pic, "button-slim"));
            componentMap[name] = envToPitchBothOscsButton.get();
        }
        if (name == "filterEnvInvertButton")
        {
            filterEnvInvertButton =
                addButton(x, y, w, h, ID::FilterEnvInvert, Name::FilterEnvInvert,
                          useAssetOrDefault(pic, "button-slim"));
            componentMap[name] = filterEnvInvertButton.get();
        }
        if (name == "osc2PWOffsetKnob")
        {
            osc2PWOffsetKnob = addKnob(x, y, w, h, d, fh, ID::Osc2PWOffset, 0.f, Name::Osc2PWOffset,
                                       useAssetOrDefault(pic, "knob"));
            componentMap[name] = osc2PWOffsetKnob.get();
        }
        if (name == "filter2PolePushButton")
        {
            filter2PolePushButton =
                addButton(x, y, w, h, ID::Filter2PolePush, Name::Filter2PolePush,
                          useAssetOrDefault(pic, "button-slim"));
            componentMap[name] = filter2PolePushButton.get();
        }
        if (name == "filter4PoleXpanderButton")
        {
            filter4PoleXpanderButton =
                addButton(x, y, w, h, ID::Filter4PoleXpander, Name::Filter4PoleXpander,
                          useAssetOrDefault(pic, "button"));
            componentMap[name] = filter4PoleXpanderButton.get();
        }
        if (name == "filterXpanderModeMenu")
        {
            if (auto list = addList(x, y, w, h, ID::FilterXpanderMode, Name::FilterXpanderMode,
                                    "menu-xpander");
                list != nullptr)
            {
                filterXpanderModeMenu = std::move(list);
                componentMap[name] = filterXpanderModeMenu.get();
            }
        }

        if (name == "patchNumberMenu")
        {
            if (auto list = addList(x, y, w, h, juce::String{}, Name::PatchList, "menu-patch");
                list != nullptr)
            {
                patchNumberMenu = std::move(list);
                componentMap[name] = patchNumberMenu.get();
            }
        }

        if (name == "prevPatchButton")
        {
            prevPatchButton = addButton(x, y, w, h, juce::String{}, Name::PrevPatch,
                                        useAssetOrDefault(pic, "button-clear"));
            componentMap[name] = prevPatchButton.get();
            auto safeThis = SafePointer(this);
            prevPatchButton->onClick = [safeThis]() {
                if (!safeThis)
                    return;
                safeThis->prevProgram();
            };
        }
        if (name == "nextPatchButton")
        {
            nextPatchButton = addButton(x, y, w, h, juce::String{}, Name::NextPatch,
                                        useAssetOrDefault(pic, "button-clear"));
            componentMap[name] = nextPatchButton.get();
            auto safeThis = SafePointer(this);
            nextPatchButton->onClick = [safeThis]() {
                if (!safeThis)
                    return;
                safeThis->nextProgram();
            };
        }

        if (name == "savePatchButton")
        {
            savePatchButton = addButton(x, y, w, h, juce::String{}, Name::SavePatch,
                                        useAssetOrDefault(pic, "button-clear-red"));
            componentMap[name] = savePatchButton.get();
            savePatchButton->onClick = [w = juce::Component::SafePointer(this)]() {
                OBLOG(rework, "We probably want save button to do something else");
            };
        }
        if (name == "origPatchButton")
        {
            origPatchButton = addButton(x, y, w, h, juce::String{}, Name::OrigPatch,
                                        useAssetOrDefault(pic, "button"));
            componentMap[name] = origPatchButton.get();
            origPatchButton->onClick = [w = juce::Component::SafePointer(this)]() {
                OBLOG(rework, "We probably want orig button to become undo/redo");
            };
        }

        if (name == "initPatchButton")
        {
            initPatchButton = addButton(x, y, w, h, juce::String{}, Name::InitializePatch,
                                        useAssetOrDefault(pic, "button-clear-red"));
            componentMap[name] = initPatchButton.get();
            auto safeThis = SafePointer(this);
            initPatchButton->onClick = [safeThis]() {
                if (!safeThis)
                    return;
                safeThis->MenuActionCallback(MenuAction::InitializePatch);
            };
        }
        if (name == "randomizePatchButton")
        {
            randomizePatchButton = addButton(x, y, w, h, juce::String{}, Name::RandomizePatch,
                                             useAssetOrDefault(pic, "button-clear-white"));
            componentMap[name] = randomizePatchButton.get();
            randomizePatchButton->onClick = [w = juce::Component::SafePointer(this)]() {
                w->randomizeCallback();
            };
        }

        if (name == "groupSelectButton")
        {
            groupSelectButton = addButton(x, y, w, h, juce::String{}, Name::PatchGroupSelect,
                                          useAssetOrDefault(pic, "button-alt"));
            componentMap[name] = groupSelectButton.get();
        }

        if (name.startsWith("select") && name.endsWith("Button"))
        {
            auto which = name.retainCharacters("0123456789").getIntValue();

            if (auto whichIdx = which - 1; whichIdx >= 0 && whichIdx < NUM_PATCHES_PER_GROUP)
            {
                selectLabels[whichIdx] =
                    addLabel(x, y, w, h, h, fmt::format("Select Group/Patch {} Label", which),
                             "button-group-patch");
                componentMap[name.replace("Button", "Label")] = selectLabels[whichIdx].get();

                selectButtons[whichIdx] = addButton(
                    x, y, w, h, juce::String{}, fmt::format("Select Group/Patch {}", which), "");

                componentMap[name] = selectButtons[whichIdx].get();

                selectButtons[whichIdx]->setTriggeredOnMouseDown(true);
                selectButtons[whichIdx]->setClickingTogglesState(false);

                auto safeThis = SafePointer(this);

                selectButtons[whichIdx]->onStateChange = [safeThis, whichIdx]() {
                    OBLOG(rework, "Select buttons do nothing");
                    if (!safeThis)
                        return;

                    if (safeThis->selectButtons[whichIdx]->isDown())
                    {
                        // auto right =
                        // juce::ModifierKeys::getCurrentModifiers().isRightButtonDown(); auto cmd =
                        // juce::ModifierKeys::getCurrentModifiers().isCommandDown();
                    }

                    safeThis->updateSelectButtonStates();
                };
            }
        }

        if (name.startsWith("lfo") && name.endsWith("SelectButton"))
        {
            auto which = name.retainCharacters("0123456789").getIntValue();

            if (const auto whichIdx = which - 1; whichIdx >= 0 && whichIdx < NUM_LFOS)
            {
                selectLFOButtons[whichIdx] =
                    addButton(x, y, w, h, juce::String{}, fmt::format("Select LFO {}", which),
                              useAssetOrDefault(pic, "button-slim"));

                componentMap[name] = selectLFOButtons[whichIdx].get();

                selectLFOButtons[whichIdx]->setRadioGroupId(1);

                auto safeThis = SafePointer(this);
                selectLFOButtons[whichIdx]->onClick = [safeThis, whichIdx]() {
                    if (!safeThis)
                        return;
                    safeThis->processor.selectedLFOIndex = whichIdx;

                    for (int i = 0; i < NUM_LFOS; i++)
                    {
                        const bool visibility = (i == whichIdx) && safeThis->selectLFOButtons[i] &&
                                                safeThis->selectLFOButtons[i]->getToggleState();
                        for (auto c : safeThis->lfoControls[i])
                        {
                            if (c)
                                c->setVisible(visibility);
                        }
                    }
                };

                if (processor.selectedLFOIndex >= 0 && processor.selectedLFOIndex < NUM_LFOS)
                {
                    if (selectLFOButtons[processor.selectedLFOIndex])
                    {
                        selectLFOButtons[processor.selectedLFOIndex]->setToggleState(
                            true, juce::sendNotification);
                        selectLFOButtons[processor.selectedLFOIndex]->triggerClick();
                    }
                }
            }
        }

        if (name == "filterModeLabel")
        {
            if (auto label = addLabel(x, y, w, h, h, "Filter Mode Label", "label-filter-mode");
                label != nullptr)
            {
                filterModeLabel = std::move(label);
                filterModeLabel->toBack();
                componentMap[name] = filterModeLabel.get();
            }
        }

        if (name == "filterOptionsLabel")
        {
            if (auto label =
                    addLabel(x, y, w, h, h, "Filter Options Label", "label-filter-options");
                label != nullptr)
            {
                filterOptionsLabel = std::move(label);
                filterOptionsLabel->toBack();
                componentMap[name] = filterOptionsLabel.get();
            }
        }

        if (name == "aboutPage")
        {
            if (auto label =
                    addButton(x, y, w, h, juce::String{}, "About Page", useAssetOrDefault(pic, ""));
                label != nullptr)
            {
                aboutPageButton = std::move(label);
                aboutPageButton->setWantsKeyboardFocus(false);
                aboutPageButton->setAccessible(false);
                componentMap[name] = aboutPageButton.get();

                auto safeThis = SafePointer(this);
                aboutPageButton->onClick = [safeThis]() {
                    if (!safeThis)
                        return;

                    safeThis->aboutScreen->showOver(safeThis.getComponent());
                };
            }
        }
    }
}

void ObxfAudioProcessorEditor::setupPolyphonyMenu() const
{
    if (polyphonyMenu)
    {
        auto *menu = polyphonyMenu->getRootMenu();

        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            if (constexpr uint8_t NUM_COLUMNS = 4;
                i > 1 && ((1 - i) % (MAX_VOICES / NUM_COLUMNS) == 0))
            {
                menu->addColumnBreak();
            }

            polyphonyMenu->addChoice(juce::String(i));
        }

        if (const auto *param = paramAdapter.getParameter(ID::Polyphony))
        {
            const auto polyOption = param->getValue();
            polyphonyMenu->setScrollWheelEnabled(true);
            polyphonyMenu->setValue(polyOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupUnisonVoicesMenu() const
{
    if (unisonVoicesMenu)
    {
        for (int i = 1; i <= MAX_PANNINGS; ++i)
        {
            unisonVoicesMenu->addChoice(juce::String(i));
        }

        if (const auto *param = paramAdapter.getParameter(ID::UnisonVoices))
        {
            const auto uniVoicesOption = param->getValue();
            unisonVoicesMenu->setScrollWheelEnabled(true);
            unisonVoicesMenu->setValue(uniVoicesOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupEnvLegatoModeMenu() const
{
    using namespace sst::plugininfra::misc_platform;

    if (envLegatoModeMenu)
    {
        envLegatoModeMenu->addChoice(toOSCase("Both Envelopes"));
        envLegatoModeMenu->addChoice(toOSCase("Filter Envelope Only"));
        envLegatoModeMenu->addChoice(toOSCase("Amplifier Envelope Only"));
        envLegatoModeMenu->addChoice(toOSCase("Always Retrigger"));
        if (const auto *param = paramAdapter.getParameter(ID::EnvLegatoMode))
        {
            const auto legatoOption = param->getValue();
            envLegatoModeMenu->setScrollWheelEnabled(true);
            envLegatoModeMenu->setValue(legatoOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupNotePriorityMenu() const
{
    if (notePriorityMenu)
    {
        notePriorityMenu->addChoice("Last");
        notePriorityMenu->addChoice("Low");
        notePriorityMenu->addChoice("High");

        if (const auto *param = paramAdapter.getParameter(ID::NotePriority))
        {
            const auto notePrioOption = param->getValue();
            notePriorityMenu->setScrollWheelEnabled(true);
            notePriorityMenu->setValue(notePrioOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupBendUpRangeMenu() const
{
    if (bendUpRangeMenu)
    {
        auto *menu = bendUpRangeMenu->getRootMenu();

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if (constexpr uint8_t NUM_COLUMNS = 4;
                (i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendUpRangeMenu->addChoice(juce::String(i));
        }

        if (const auto *param = paramAdapter.getParameter(ID::BendUpRange))
        {
            const auto bendUpOption = param->getValue();
            bendUpRangeMenu->setScrollWheelEnabled(true);
            bendUpRangeMenu->setValue(bendUpOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupBendDownRangeMenu() const
{
    if (bendDownRangeMenu)
    {
        auto *menu = bendDownRangeMenu->getRootMenu();

        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            if (constexpr uint8_t NUM_COLUMNS = 4;
                (i > 0 && (i - 1) % (MAX_BEND_RANGE / NUM_COLUMNS) == 0) || i == 1)
            {
                menu->addColumnBreak();
            }

            bendDownRangeMenu->addChoice(juce::String(i));
        }

        if (const auto *param = paramAdapter.getParameter(ID::BendDownRange))
        {
            const auto bendDownOption = param->getValue();
            bendDownRangeMenu->setScrollWheelEnabled(true);
            bendDownRangeMenu->setValue(bendDownOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupFilterXpanderModeMenu() const
{
    if (filterXpanderModeMenu)
    {
        filterXpanderModeMenu->addChoice("LP4");
        filterXpanderModeMenu->addChoice("LP3");
        filterXpanderModeMenu->addChoice("LP2");
        filterXpanderModeMenu->addChoice("LP1");
        filterXpanderModeMenu->addChoice("HP3");
        filterXpanderModeMenu->addChoice("HP2");
        filterXpanderModeMenu->addChoice("HP1");
        filterXpanderModeMenu->addChoice("BP4");
        filterXpanderModeMenu->addChoice("BP2");
        filterXpanderModeMenu->addChoice("N2");
        filterXpanderModeMenu->addChoice("PH3");
        filterXpanderModeMenu->addChoice("HP2+LP1");
        filterXpanderModeMenu->addChoice("HP3+LP1");
        filterXpanderModeMenu->addChoice("N2+LP1");
        filterXpanderModeMenu->addChoice("PH3+LP1");

        if (const auto *param = paramAdapter.getParameter(ID::FilterXpanderMode))
        {
            const auto xpanderModeOption = param->getValue();
            filterXpanderModeMenu->setScrollWheelEnabled(true);
            filterXpanderModeMenu->setValue(xpanderModeOption, juce::dontSendNotification);
        }
    }
}
void ObxfAudioProcessorEditor::setupPatchNumberMenu()
{
    if (patchNumberMenu)
    {
        auto *menu = patchNumberMenu->getRootMenu();

        menu->clear();
        createPatchList(*menu);
        patchNumberMenu->setSelectedId(0, juce::dontSendNotification);
    }
}

void ObxfAudioProcessorEditor::setupMenus()
{
    setupPolyphonyMenu();
    setupUnisonVoicesMenu();
    setupEnvLegatoModeMenu();
    setupNotePriorityMenu();
    setupBendUpRangeMenu();
    setupBendDownRangeMenu();
    setupFilterXpanderModeMenu();
    setupPatchNumberMenu();
    createMenu();
}

void ObxfAudioProcessorEditor::restoreComponentParameterValues(
    const std::map<juce::String, float> &parameterValues)
{
    for (auto &[paramId, paramValue] : parameterValues)
    {
        if (componentMap.find(paramId) != componentMap.end())
        {
            Component *comp = componentMap[paramId];
            if (auto *knob = dynamic_cast<Knob *>(comp))
                knob->setValue(paramValue, juce::dontSendNotification);
            else if (auto *multiState = dynamic_cast<MultiStateButton *>(comp))
                multiState->setValue(paramValue, juce::dontSendNotification);
            else if (auto *button = dynamic_cast<ToggleButton *>(comp))
                button->setToggleState(paramValue > 0.5f, juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::finalizeThemeLoad(ObxfAudioProcessor &ownerFilter)
{
    ownerFilter.addChangeListener(this);
    skinLoaded = true;
    scaleFactorChanged();
    repaint();
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
#if defined(DEBUG) || defined(_DEBUG)
    if (inspector)
        inspector.reset();
#endif

    focusDebugger.reset();

    processor.uiState.editorAttached = false;
    idleTimer->stopTimer();
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    mainMenu.reset();
    popupMenus.clear();
    componentMap.clear();

    for (auto &controls : lfoControls)
        controls.clear();

    juce::PopupMenu::dismissAllActiveMenus();
}

void ObxfAudioProcessorEditor::idle()
{
    if (!skinLoaded)
    {
        return;
    }

    countTimer++;

    if (isShowing() && isVisible() && resizeOnNextIdle >= 0)
    {
        resizeOnNextIdle--;
        if (resizeOnNextIdle == 0)
        {
            resized();

            // including forcing the larger assets to load if needed
            for (auto &[n, c] : componentMap)
            {
                if (auto hcf = dynamic_cast<HasScaleFactor *>(c))
                {
                    hcf->scaleFactorChanged();
                }
            }
        }
    }

    if (countTimer == 4 && needNotifyToHost)
    {
        countTimer = 0;
        needNotifyToHost = false;
        processor.updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));

        if (patchNumberMenu)
        {
            setupPatchNumberMenu();
            patchNumberMenu->setSelectedId(0, juce::dontSendNotification);
        }
    }

    if (midiLearnButton)
    {
        midiLearnButton->setToggleState(paramAdapter.midiLearnAttachment.get(),
                                        juce::dontSendNotification);
    }

    if (polyphonyMenu != nullptr)
    {
        int curPoly = juce::jmin(polyphonyMenu->getSelectedId(), MAX_VOICES);

        // only show the exact number of LEDs as we have set polyphony voices
        for (int i = 0; i < MAX_VOICES; i++)
        {
            if (voiceLEDs[i] && voiceBGs[i])
            {
                if (i >= curPoly && voiceBGs[i]->isVisible())
                {
                    voiceBGs[i]->setVisible(false);
                    voiceLEDs[i]->setVisible(false);
                }

                if (i < curPoly && !voiceBGs[i]->isVisible())
                {
                    voiceBGs[i]->setVisible(true);
                    voiceLEDs[i]->setVisible(true);
                }
            }
        }

        for (int i = 0; i < curPoly; i++)
        {
            const auto state = static_cast<float>(processor.uiState.voiceStatusValue[i]);

            if (voiceLEDs[i])
            {
                voiceLEDs[i]->setAlpha(std::cbrtf(state));
            }
        }

        for (int i = curPoly; i < MAX_VOICES; i++)
        {
            if (voiceLEDs[i] && voiceLEDs[i]->getAlpha() > 0.f)
            {
                voiceLEDs[i]->setAlpha(0.f);
            }
        }
    }

    if (osc1TriangleLabel && osc1SawButton && osc1PulseButton)
    {
        osc1TriangleLabel->setCurrentFrame(
            !(osc1SawButton->getToggleState() || osc1PulseButton->getToggleState()));
    }

    if (osc2TriangleLabel && osc2SawButton && osc2PulseButton)
    {
        osc2TriangleLabel->setCurrentFrame(
            !(osc2SawButton->getToggleState() || osc2PulseButton->getToggleState()));
    }

    if (osc1PulseLabel && osc2PulseLabel)
    {
        const auto pw1 = juce::roundToInt(oscPWKnob->getValue() * 46.f);
        const auto pw2 =
            juce::jmin(pw1 + juce::roundToInt(osc2PWOffsetKnob->getValue() * 46.f), 50);

        osc1PulseLabel->setCurrentFrame(pw1);
        osc2PulseLabel->setCurrentFrame(pw2);
    }

    if (lfo1Wave2Label && lfo1Wave2Label->isVisible())
    {
        const auto state = juce::roundToInt(lfo1PWSlider->getValue() * 24.f);

        if (lfo1Wave2Label && state != lfo1Wave2Label->getCurrentFrame())
        {
            lfo1Wave2Label->setCurrentFrame(state);
        }
    }

    if (lfo2Wave2Label && lfo2Wave2Label->isVisible())
    {
        const auto state = juce::roundToInt(lfo2PWSlider->getValue() * 24.f);

        if (lfo2Wave2Label && state != lfo2Wave2Label->getCurrentFrame())
        {
            lfo2Wave2Label->setCurrentFrame(state);
        }
    }

    const auto fourPole = filter4PoleModeButton && filter4PoleModeButton->getToggleState();
    const auto xpanderMode = filter4PoleXpanderButton && filter4PoleXpanderButton->getToggleState();
    const auto bpBlend = filter2PoleBPBlendButton && filter2PoleBPBlendButton->getToggleState();

    const auto filterModeFrame = fourPole ? (xpanderMode ? 3 : 2) : (bpBlend ? 1 : 0);

    if (filterModeLabel && filterModeFrame != filterModeLabel->getCurrentFrame())
    {
        filterModeLabel->setCurrentFrame(filterModeFrame);
    }

    if (filterOptionsLabel && fourPole != filterOptionsLabel->getCurrentFrame())
    {
        filterOptionsLabel->setCurrentFrame(fourPole);
    }

    if (filter2PoleBPBlendButton)
    {
        filter2PoleBPBlendButton->setVisible(!fourPole);
    }

    if (filter2PolePushButton)
    {
        filter2PolePushButton->setVisible(!fourPole);
    }

    if (filter4PoleXpanderButton)
    {
        filter4PoleXpanderButton->setVisible(fourPole);
    }

    if (filterModeKnob)
    {
        filterModeKnob->setVisible(!(fourPole && xpanderMode));
    }

    if (filterXpanderModeMenu)
    {
        filterXpanderModeMenu->setVisible(fourPole && xpanderMode);
    }

    if (unisonButton && unisonVoicesMenu)
    {
        if (unisonButton->getToggleState() && unisonVoicesMenu->getAlpha() < 1.f)
        {
            unisonVoicesMenu->setAlpha(1.f);
        }

        if (!unisonButton->getToggleState() && unisonVoicesMenu->getAlpha() == 1.f)
        {
            unisonVoicesMenu->setAlpha(0.25f);
        }
    }

    updateSelectButtonStates();

    if (patchNameLabel && !patchNameLabel->isBeingEdited())
    {
        patchNameLabel->setText(processor.getActiveProgram().getName(), juce::dontSendNotification);
    }

    if (origPatchButton)
    {
        if (!origPatchButton->isMouseButtonDown())
        {
            OBLOGONCE(rework, "Still polling dirty in idle");
            bool isDirty = false;
            auto val = origPatchButton->getToggleState();

            if (val != isDirty)
            {
                origPatchButton->setToggleState(isDirty, juce::dontSendNotification);
            }

            origPatchButton->setEnabled(isDirty);
        }
    }
}

void ObxfAudioProcessorEditor::scaleFactorChanged()
{
    if (imageCache.isSVG("background"))
    {
        backgroundIsSVG = true;
    }
    else
    {
        backgroundIsSVG = false;
        backgroundImage = imageCache.getImageFor("background", getWidth(), getHeight());
    }
    repaint();
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

    auto *label = new Label(assetName, fh, imageCache);

    label->setBounds(transformBounds(x, y, w, h));
    label->setName(name);

    addAndMakeVisible(label);

    return std::unique_ptr<Label>(label);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(int x, int y, int w, int h, int d, int fh,
                                                        const juce::String &paramId, float defval,
                                                        const juce::String &name,
                                                        const juce::String &assetName)
{
    std::unique_ptr<Knob> knob;

    if (fh == 0)
    {
        int frameHeight = defKnobDiameter;
        if (d > 0)
            frameHeight = d;
        else if (fh > 0)
            frameHeight = fh;

        knob = std::make_unique<Knob>(assetName, frameHeight, &processor, imageCache);
    }
    else
    {
        knob = std::make_unique<Knob>(assetName, fh, &processor, imageCache);
        if (w > h)
            knob->svgTranslationMode = Knob::HORIZONTAL;
        else
            knob->svgTranslationMode = Knob::VERTICAL;
    }

    if (!paramId.isEmpty())
    {
        if (auto *param = paramAdapter.getParameter(paramId); param != nullptr)
        {
            knob->setParameter(param);
            knob->setValue(param->getValue());
            knobAttachments.emplace_back(new KnobAttachment(
                paramAdapter.getParameterManager(), param, *knob,
                [](Knob &k, float v) { k.setValue(v, juce::dontSendNotification); },
                [](const Knob &k) { return static_cast<float>(k.getValue()); }));
        }

        knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);

        if (d > 0)
        {
            knob->setBounds(transformBounds(x, y, d, d));
            if (w > 0 && h > 0 && w > h)
            {
                knob->setSliderStyle(juce::Slider::RotaryHorizontalDrag);
            }
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
    }

    addAndMakeVisible(*knob);
    return knob;
}

std::unique_ptr<ToggleButton> ObxfAudioProcessorEditor::addButton(const int x, const int y,
                                                                  const int w, const int h,
                                                                  const juce::String &paramId,
                                                                  const juce::String &name,
                                                                  const juce::String &assetName)
{
    auto *button = new ToggleButton(assetName, h, imageCache, &processor);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramAdapter.getParameter(paramId))
        {
            button->setParameter(param);
            button->setToggleState(param->getValue() > 0.5f, juce::dontSendNotification);
            toggleAttachments.emplace_back(new ButtonAttachment(
                paramAdapter.getParameterManager(), param, *button,
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

    button->setBounds(transformBounds(x, y, w, h));
    button->setButtonText(name);
    button->setTitle(name);
    button->setTriggeredOnMouseDown(true);

    addAndMakeVisible(button);

    return std::unique_ptr<ToggleButton>(button);
}

std::unique_ptr<MultiStateButton> ObxfAudioProcessorEditor::addMultiStateButton(
    const int x, const int y, const int w, const int h, const juce::String &paramId,
    const juce::String &name, const juce::String &assetName, const uint8_t numStates)
{
    auto *button = new MultiStateButton(assetName, imageCache, &processor, numStates);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramAdapter.getParameter(paramId); param != nullptr)
        {
            button->setOptionalParameter(param);
            button->setValue(param->getValue(), juce::dontSendNotification);
            multiStateAttachments.emplace_back(new MultiStateAttachment(
                paramAdapter.getParameterManager(), param, *button,
                [](MultiStateButton &b, float v) { b.setValue(v, juce::dontSendNotification); },
                [](const MultiStateButton &b) { return static_cast<float>(b.getValue()); }));
        }

        button->setBounds(transformBounds(x, y, w, h));
        button->setTitle(name);
    }

    addAndMakeVisible(button);

    return std::unique_ptr<MultiStateButton>(button);
}

std::unique_ptr<ButtonList> ObxfAudioProcessorEditor::addList(const int x, const int y, const int w,
                                                              const int h,
                                                              const juce::String &paramId,
                                                              const juce::String &name,
                                                              const juce::String &assetName)
{
    auto *list = new ButtonList(assetName, h, imageCache, &processor);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramAdapter.getParameter(paramId); param != nullptr)
        {
            list->setParameter(param);
            list->setValue(param->getValue(), juce::dontSendNotification);

            buttonListAttachments.emplace_back(new ButtonListAttachment(
                paramAdapter.getParameterManager(), param, *list,
                [](ButtonList &k, float v) { k.setValue(v, juce::dontSendNotification); },
                [](const ButtonList &k) { return static_cast<float>(k.getValue()); }));
        }
    }

    list->setBounds(transformBounds(x, y, w, h));
    list->setName(name);
    list->setTitle(name);

    addAndMakeVisible(list);

    return std::unique_ptr<ButtonList>(list);
}

std::unique_ptr<ImageMenu> ObxfAudioProcessorEditor::addMenu(const int x, const int y, const int w,
                                                             const int h,
                                                             const juce::String &assetName)
{
    auto *menu = new ImageMenu(assetName, imageCache);

    menu->setBounds(transformBounds(x, y, w, h));
    menu->setName("Menu");

    auto safeThis = SafePointer(this);
    menu->onClick = [safeThis]() {
        if (!safeThis)
            return;
        if (safeThis->mainMenu)
        {
            auto x = safeThis->mainMenu->getScreenX();
            auto y = safeThis->mainMenu->getScreenY();
            auto dx = safeThis->mainMenu->getWidth();
            auto pos = juce::Point<int>(x, y + dx);

            safeThis->resultFromMenu(pos);
        }
    };

    addAndMakeVisible(menu);

    return std::unique_ptr<ImageMenu>(menu);
}

void ObxfAudioProcessorEditor::actionListenerCallback(const juce::String & /*message*/) {}

juce::Rectangle<int> ObxfAudioProcessorEditor::transformBounds(int x, int y, int w, int h) const
{
    if (initialBounds.isEmpty())
        return {x, y, w, h};

    auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
    auto currentAspectRatio = 1.f * getWidth() / getHeight();
    float scale{1.f};

    if (currentAspectRatio > aspectRatio)
    {
        // this means the width is too big
        scale = static_cast<float>(getHeight()) / static_cast<float>(initialBounds.getHeight());
    }
    else
    {
        // this means the height is too big
        scale = static_cast<float>(getWidth()) / static_cast<float>(initialBounds.getWidth());
    }

    return {juce::roundToInt(static_cast<float>(x) * scale),
            juce::roundToInt(static_cast<float>(y) * scale),
            juce::roundToInt(static_cast<float>(w) * scale),
            juce::roundToInt(static_cast<float>(h) * scale)};
}

juce::String ObxfAudioProcessorEditor::useAssetOrDefault(const juce::String &assetName,
                                                         const juce::String &defaultAssetName) const
{
    if (assetName.isNotEmpty())
        return assetName;
    else
        return defaultAssetName;
}

void ObxfAudioProcessorEditor::clean()
{
    this->removeAllChildren();

    if (aboutScreen)
    {
        addChildComponent(*aboutScreen);
    }
}

void ObxfAudioProcessorEditor::rebuildComponents(ObxfAudioProcessor &ownerFilter)
{
    themeLocation = utils.getCurrentThemeLocation();

    ownerFilter.removeChangeListener(this);

    setSize(1440, 450);

    ownerFilter.addChangeListener(this);
    repaint();
}

juce::PopupMenu ObxfAudioProcessorEditor::createPatchList(juce::PopupMenu &menu) const
{
    auto raddTo = [that = this](juce::PopupMenu &m, Utils::PatchTreeNode &node,
                                auto &&self) -> void {
        for (auto &child : node.children)
        {
            if (child.isFolder)
            {
                if (!child.children.empty())
                {
                    juce::PopupMenu subMenu;
                    self(subMenu, child, self);
                    m.addSubMenu(child.displayName, subMenu);
                }
            }
            else
            {
                m.addItem(child.displayName, true, false,
                          [fn = child.file, w = that]() { w->utils.loadPatch(fn); });
            }
        }
    };

    for (auto i = 0U; i < utils.patchRoot.children.size(); i++)
    {
        auto &ch = utils.patchRoot.children[i];
        menu.addSectionHeader(Utils::toString(ch.locationType));
        raddTo(menu, ch, raddTo);
        if (i < utils.patchRoot.children.size() - 1)
            menu.addSeparator();
    }

    return menu;
}

void ObxfAudioProcessorEditor::createMenu()
{
    using namespace sst::plugininfra::misc_platform;

    bool enablePasteOption = utils.isPatchInClipboard();
    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    themes = utils.getThemeLocations();

    {
        juce::PopupMenu fileMenu;

        fileMenu.addItem(static_cast<int>(MenuAction::InitializePatch),
                         toOSCase("Initialize Patch"), true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(MenuAction::ImportPatch, toOSCase("Load Patch..."), true, false);
        fileMenu.addItem(MenuAction::ExportPatch, toOSCase("Save Patch..."), true, false);

        fileMenu.addSeparator();

        fileMenu.addItem(MenuAction::CopyPatch, toOSCase("Copy Patch"), true, false);
        fileMenu.addItem(MenuAction::PastePatch, toOSCase("Paste Patch"), enablePasteOption, false);

        fileMenu.addSeparator();
        fileMenu.addItem(MenuAction::RevealUserDirectory, toOSCase("Open User Data Folder..."),
                         true, false);
        menu->addSubMenu("File", fileMenu);
    }

    {
        juce::PopupMenu patchesMenu;
        menu->addSubMenu("Patches", createPatchList(patchesMenu));
    }

    createMidiMapMenu(static_cast<int>(midiStart), midiMenu);

    menu->addSubMenu(toOSCase("MIDI Mappings"), midiMenu);

    {
        juce::PopupMenu themeMenu;
        auto ll = Utils::LocationType::SYSTEM_FACTORY;
        if (themes.size() && themes[0].locationType != Utils::LocationType::USER)
        {
            themeMenu.addSectionHeader("Factory");
        }
        for (size_t i = 0; i < themes.size(); ++i)
        {
            auto theme = themes[i];
            if (theme.locationType != ll && theme.locationType == Utils::LocationType::USER)
            {
                if (i != 0)
                    themeMenu.addSeparator();
                themeMenu.addSectionHeader("User");
            }
            ll = theme.locationType;

            themeMenu.addItem(static_cast<int>(i + themeStart + 1), theme.file.getFileName(), true,
                              theme == themeLocation);
        }

#if JUCE_WINDOWS
        themeMenu.addSeparator();
        auto usw = utils.getUseSoftwareRenderer();
        themeMenu.addItem(toOSCase("Use Software Renderer"), true, usw,
                          [w = juce::Component::SafePointer(this)]() {
                              if (!w)
                                  return;
                              auto usw = w->utils.getUseSoftwareRenderer();
                              w->utils.setUseSoftwareRenderer(!usw);
                              juce::AlertWindow::showMessageBoxAsync(
                                  juce::AlertWindow::WarningIcon, "Software Renderer Change",
                                  "A software renderer change is only active "
                                  "once you restart/reload the plugin.");
                          });
#endif

        menu->addSubMenu("Themes", themeMenu);
    }

    {
        juce::PopupMenu sizeMenu;

        bool isCurZoomAmongScaleFactors = false;
        auto curZoom = impliedScaleFactor();
        auto pd = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

        juce::Rectangle<float> dispArea{0, 0, 100000, 100000};
        if (pd)
        {
            // when would this code not have a primary display?
            // when the juce LV2 lets-print-our-state-at-build-time
            // nonsense runs ahd hits load theme because it makes an
            // editor without an xvfb. And then crashes our pipelines.
            dispArea = pd->userArea.toFloat();
        }

        for (int i = 0; i < numScaleFactors; i++)
        {
            auto scaledArea = initialBounds.toFloat().transformedBy(
                juce::AffineTransform().scaled(scaleFactors[i]));

            bool isActive = dispArea.getWidth() >= scaledArea.getWidth() &&
                            dispArea.getHeight() >= scaledArea.getHeight();

            bool isTicked = std::fabs(scaleFactors[i] - curZoom) * 100 < 0.5f;

            if (isTicked && !isCurZoomAmongScaleFactors)
            {
                isCurZoomAmongScaleFactors = true;
            }

            sizeMenu.addItem(static_cast<int>(sizeStart) + i,
                             fmt::format("{:.0f}%", scaleFactors[i] * 100.f), isActive, isTicked);
        }

        sizeMenu.addSeparator();

        if (!isCurZoomAmongScaleFactors)
        {
            sizeMenu.addItem(
                -1, toOSCase(fmt::format("Custom Zoom Level: {:.{}f}%", curZoom * 100.f, 1)), false,
                true);
        }

        // used to be an if - change to show it non-enabled
        // if (utils.getDefaultZoomFactor() != curZoom)
        {
            auto disp = (utils.getDefaultZoomFactor() != curZoom);
            auto dispZoom = curZoom;

            if (isCurZoomAmongScaleFactors)
            {
                dispZoom = std::round(curZoom * 100.f) / 100.f;
            }

            sizeMenu.addItem(
                toOSCase(fmt::format("Set {:.{}f}% as Default Zoom Level", dispZoom * 100.f,
                                     isCurZoomAmongScaleFactors ? 0 : 1)),
                disp, false, [this, curZoom]() { utils.setDefaultZoomFactor(curZoom); });
        }

        menu->addSubMenu("Zoom", sizeMenu);
    }

#if defined(DEBUG) || defined(_DEBUG)
    juce::PopupMenu debugMenu;

    debugMenu.addSeparator();

    const bool isInspectorVisible = inspector && inspector->isVisible();

    debugMenu.addItem(MenuAction::Inspector, toOSCase("GUI Inspector..."), true,
                      isInspectorVisible);
    debugMenu.addItem(toOSCase("Toggle Focus Debugger"),
                      [w = juce::Component::SafePointer(this)]() {
                          if (w)
                              w->focusDebugger->setDoFocusDebug(!w->focusDebugger->doFocusDebug);
                      });

    menu->addSubMenu("Developer", debugMenu);
#endif

    menu->addSeparator();

    menu->addItem("About OB-Xf", [w = SafePointer(this)]() {
        if (!w)
            return;
        w->aboutScreen->showOver(w.getComponent());
    });

    popupMenus.emplace_back(menu);
}

void ObxfAudioProcessorEditor::createMidiMapMenu(int menuNo, juce::PopupMenu &menuMidi)
{
    using namespace sst::plugininfra::misc_platform;

    menuMidi.addItem(toOSCase("Clear MIDI Mapping"), true, false,
                     [this]() { processor.getMidiMap().reset(); });

    menuMidi.addSeparator();

    const juce::File midi_dir = utils.getMidiFolderFor(Utils::LocationType::USER);

    juce::StringArray list;

    for (const auto &entry : juce::RangedDirectoryIterator(midi_dir, true, "*.xml"))
    {
        list.add(entry.getFile().getFullPathName());
    }

    list.sort(true);

    for (const auto &i : list)
    {
        juce::File f(i);
        auto name = f.getFileNameWithoutExtension();

        bool isCurrent = (processor.getCurrentMidiPath() == f.getFullPathName());

        menuMidi.addItem(menuNo++, name, true, isCurrent);
        midiFiles.emplace_back(
            Utils::MidiLocation{Utils::LocationType::USER, midi_dir.getFileName(), f});
    }
}

void ObxfAudioProcessorEditor::resultFromMenu(const juce::Point<int> pos)
{
    createMenu();

    popupMenus[0]->showMenuAsync(
        juce::PopupMenu::Options().withTargetScreenArea(
            juce::Rectangle<int>(pos.getX(), pos.getY(), 1, 1)),
        [this](size_t result) {
            if (result >= (themeStart + 1) && result <= (themeStart + themes.size()))
            {
                result -= 1;
                result -= themeStart;

                utils.setCurrentThemeLocation(themes[result]);
                themeLocation = themes[result];
                clean();
                loadTheme(processor);
            }
            else if (result >= sizeStart && result < (sizeStart + numScaleFactors))
            {
                size_t index = result - sizeStart;
                const int newWidth =
                    juce::roundToInt(static_cast<float>(initialWidth) * scaleFactors[index]);
                const int newHeight =
                    juce::roundToInt(static_cast<float>(initialHeight) * scaleFactors[index]);

                setSize(newWidth, newHeight);
                resized();
            }
            else if (result < midiStart)
            {
                MenuActionCallback(static_cast<int>(result));
            }
            else if (result >= midiStart)
            {
                if (const auto selected_idx = result - midiStart; selected_idx < midiFiles.size())
                {
                    const auto &midiLoc = midiFiles[selected_idx];

                    if (juce::File f = midiLoc.file; f.exists())
                    {
                        processor.getCurrentMidiPath() = f.getFullPathName();
                        processor.getMidiMap().loadFile(f);
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
        processor.processActiveProgramChanged();
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
                    needNotifyToHost = true;
                    countTimer = 0;
                }
            });
    }

    if (action == MenuAction::ExportPatch)
    {
        const auto file = utils.getPresetsFolder().getChildFile(
            fmt::format("{}.fxp", processor.getActiveProgram().getName().toStdString()));
        fileChooser = std::make_unique<juce::FileChooser>("Export Patch", file, "*.fxp", true);
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
        utils.copyPatch();
    }

    // Paste from clipboard
    if (action == MenuAction::PastePatch)
    {
        utils.pastePatch();
    }

    if (action == MenuAction::RevealUserDirectory)
    {
        utils.getDocumentFolder().revealToUser();
    }

#if defined(DEBUG) || defined(_DEBUG)
    // Open Melatonin Inspector
    if (action == MenuAction::Inspector)
    {
        this->inspector->setVisible(!this->inspector->isVisible());
        this->inspector->toggle(this->inspector->isVisible());
    }
#endif
}

void ObxfAudioProcessorEditor::nextProgram() { OBLOG(rework, "Unimplemented : " << __func__); }

void ObxfAudioProcessorEditor::prevProgram() { OBLOG(rework, "Unimplemented : " << __func__); }

void ObxfAudioProcessorEditor::updateFromHost()
{
    for (const auto &knobAttachment : knobAttachments)
    {
        knobAttachment->updateToControl();
    }
    for (const auto &buttonListAttachment : buttonListAttachments)
    {
        buttonListAttachment->updateToControl();
    }
    for (const auto &toggleAttachment : toggleAttachments)
    {
        toggleAttachment->updateToControl();
    }
    for (const auto &multiStateAttachment : multiStateAttachments)
    {
        multiStateAttachment->updateToControl();
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

void ObxfAudioProcessorEditor::paintMissingAssets(juce::Graphics &g)
{
    g.fillAll(juce::Colour(30, 30, 50));
    auto h{20}, p{4};
    g.setFont(juce::FontOptions(h));
    g.setColour(juce::Colours::white);
    auto b = getLocalBounds().reduced(5).withHeight(h);

    auto write = [&](auto m) {
        g.drawText(m, b, juce::Justification::centred, true);
        b = b.translated(0, h + p);
    };

    write("Missing Assets");
    write("Please check your installation.");
    write("");
    write("Asset Directory List is");
    g.setFont(juce::FontOptions(15));
    write(utils.getSystemFactoryFolder().getFullPathName());
    write(utils.getLocalFactoryFolder().getFullPathName());
    write(utils.getDocumentFolder().getFullPathName());
    g.setFont(juce::FontOptions(h));
    write("");
    write("Current Theme Directory is");
    g.setFont(juce::FontOptions(15));
    write(themeLocation.file.getFullPathName());
    g.setFont(juce::FontOptions(h));
    write("");
    write("You can get the assets from our GitHub repo to place there.");
    write("If you run the macOS/Windows installer you shouldn't see this any more. Linux soon!");
}

void ObxfAudioProcessorEditor::paint(juce::Graphics &g)
{
    if (noThemesAvailable)
    {
        paintMissingAssets(g);
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
    if (backgroundIsSVG)
    {
        const auto &bgi = imageCache.getSVGDrawable("background");
        if (bgi)
        {
            auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
            auto currentAspectRatio = 1.f * getWidth() / getHeight();
            float scale{1.f};

            if (currentAspectRatio > aspectRatio)
            {
                // this means the width is too big
                scale =
                    static_cast<float>(getHeight()) / static_cast<float>(initialBounds.getHeight());
            }
            else
            {
                // this means the height is too big
                scale =
                    static_cast<float>(getWidth()) / static_cast<float>(initialBounds.getWidth());
            }
            auto tf = juce::AffineTransform().scaled(scale);
            bgi->draw(g, 1.f, tf);
        }
        else
        {
            OBLOG(themes, "Background SVG is not present");
            g.fillAll(juce::Colour(50, 0, 0));
        }
    }
    else
    {
        auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
        auto currentAspectRatio = 1.f * getWidth() / getHeight();

        auto w = getWidth();
        auto h = getHeight();
        if (currentAspectRatio > aspectRatio)
        {
            // this means the width is too big
            w = getHeight() * aspectRatio;
        }
        else
        {
            // this means the height is too big
            h = getWidth() / aspectRatio;
        }
        g.drawImage(backgroundImage, 0, 0, w, h, 0, 0, backgroundImage.getWidth(),
                    backgroundImage.getHeight());
    }
}

bool ObxfAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() == 1)
    {
        const auto file = juce::File(files[0]);
        const juce::String ext = file.getFileExtension().toLowerCase();
        return file.existsAsFile() && (ext == ".fxp" || ext == ".fxb");
    }

    return std::any_of(files.begin(), files.end(), [](const juce::String &q) {
        const juce::File file(q);
        const juce::String ext = file.getFileExtension().toLowerCase();
        return ext == ".fxb" || ext == ".fxp";
    });
}

void ObxfAudioProcessorEditor::filesDropped(const juce::StringArray &files, int /*x*/, int /*y*/)
{
    if (files.size() == 1)
    {
        const auto file = juce::File(files[0]);

        if (const juce::String ext = file.getFileExtension().toLowerCase(); ext == ".fxp")
        {
            OBLOG(patches, "Dropped an fxp " << file.getFullPathName());
            utils.loadPatch(file);
            processor.processActiveProgramChanged();
            needNotifyToHost = true;
            countTimer = 0;
        }
    }
    else
    {
        OBLOG(rework, "Multi-patch drop - wht does it even mean any more?");
    }
}

float ObxfAudioProcessorEditor::impliedScaleFactor() const
{
    auto xf = getWidth() / static_cast<float>(initialWidth);
    auto yf = getHeight() / static_cast<float>(initialHeight);

    return std::max(xf, yf);
}

void ObxfAudioProcessorEditor::keyboardFocusMainMenu()
{
    auto mmit = componentMap.find("mainMenu");
    if (isShowing() && mmit != componentMap.end() && mmit->second->isShowing())
    {
        mmit->second->setWantsKeyboardFocus(true);
        juce::Timer::callAfterDelay(50, [w = juce::Component::SafePointer(mmit->second)]() {
            if (w)
                w->grabKeyboardFocus();
        });
    }
    else
    {
        juce::Timer::callAfterDelay(30, [w = juce::Component::SafePointer(this)]() {
            if (w)
                w->keyboardFocusMainMenu();
        });
    }
}

void ObxfAudioProcessorEditor::setScaleFactor(float newScale)
{
    utils.setPluginAPIScale(newScale);
    // this line causes the crash with bitmap assets we've been chasing
    // WHy? We kinda need it I think...
    // AudioProcessorEditor::setScaleFactor(newScale);
}

void ObxfAudioProcessorEditor::randomizeCallback()
{
#if ALLOW_RANDOM_VARIATIONS
    auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();

    if (mods.isPopupMenu())
    {
        auto m = juce::PopupMenu();
        m.addSectionHeader("Randomizer");
        m.addSeparator();
        for (auto [name, alg] : {std::make_pair("A Little", A_SMIDGE),
                                 {"Medium", A_BIT_MORE},
                                 {"Full", EVERYTHING},
                                 {"", EVERYTHING},
                                 {"Pans", PANS}})
        {
            if (name[0] == 0)
                m.addSeparator();
            else
                m.addItem(name, [alg, w = juce::Component::SafePointer(this)]() {
                    if (w)
                        w->processor.randomizeToAlgo(alg);
                });
        }
        m.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
    }
    else
    {
        processor.randomizeToAlgo(EVERYTHING);
    }
#else
    processor.randomizeToAlgo(A_BIT_MORE);
#endif
}
