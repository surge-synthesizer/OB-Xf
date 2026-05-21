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

#include "../ObxfEditor.h"

#include "gui/AboutScreen.h"
#include "gui/MPEMatrix.h"
#include "gui/SaveDialog.h"

// Theme lifecycle
void ObxfAudioProcessorEditor::loadTheme(ObxfAudioProcessor &ownerFilter)
{
    skinLoaded = false;

    OBLOG(themes, "Setting theme to " << themeLocation.file.getFullPathName()
                                      << " (type=" << (int)themeLocation.locationType << ")");

    const auto parameterValues = saveComponentParameterValues();
    clearAndResetComponents(ownerFilter);

    if (themeLocation.locationType == Utils::EMBEDDED)
    {
        auto xml = juce::String(BinaryData::theme_xml, BinaryData::theme_xmlSize);
        cachedThemeXml = juce::XmlDocument(xml).getDocumentElement();
    }
    else
    {
        const juce::File theme = themeLocation.file.getChildFile("theme.xml");
        cachedThemeXml = juce::XmlDocument(theme).getDocumentElement();
    }

    if (!cachedThemeXml)
    {
        jassertfalse;
        return;
    }

    if (cachedThemeXml->getTagName() == "obxf-theme")
    {
        imageCache.clearCache();
        imageCache.embeddedMode = (themeLocation.locationType == Utils::EMBEDDED);
        imageCache.skinDir = imageCache.embeddedMode ? juce::File() : themeLocation.file;

        createComponentsFromXml(cachedThemeXml.get());
    }

    setupMenus();
    restoreComponentParameterValues(parameterValues);
    finalizeThemeLoad(ownerFilter);
    resized();
}

std::map<juce::String, float> ObxfAudioProcessorEditor::saveComponentParameterValues()
{
    std::map<juce::String, float> values;

    for (auto &[name, comp] : componentMap)
    {
        if (const auto *k = dynamic_cast<Knob *>(comp.get()))
        {
            values[name] = static_cast<float>(k->getValue());
        }
        else if (const auto *ms = dynamic_cast<MultiStateButton *>(comp.get()))
        {
            values[name] = static_cast<float>(ms->getValue());
        }
        else if (const auto *b = dynamic_cast<ToggleButton *>(comp.get()))
        {
            values[name] = b->getToggleState() ? 1.f : 0.f;
        }
    }

    return values;
}

void ObxfAudioProcessorEditor::clearAndResetComponents(ObxfAudioProcessor &ownerFilter)
{
    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    popupMenus.clear();
    cachedLayout.clear();
    componentByParamID.clear();
    panelGroups.clear();
    updateFilterVisibility = nullptr;

    // Remove all child components that are in componentMap before clearing it
    for (auto &[name, comp] : componentMap)
    {
        if (comp)
        {
            removeChildComponent(comp.get());
        }
    }

    componentMap.clear();

    ownerFilter.removeChangeListener(this);

    themeLocation = utils.getCurrentThemeLocation();
}

bool ObxfAudioProcessorEditor::parseAndCreateComponentsFromTheme()
{
    const juce::File theme = themeLocation.file.getChildFile("theme.xml");

    if (!theme.existsAsFile())
    {
        storeWidget(componentMap, this, "mainMenu", addMenu(14, 25, 23, 35, "button-clear-red"));
        rebuildComponents(processor);

        return false;
    }

    juce::XmlDocument themeXml(theme);

    if (const auto doc = themeXml.getDocumentElement(); !doc)
    {
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

void ObxfAudioProcessorEditor::restoreComponentParameterValues(
    const std::map<juce::String, float> &parameterValues)
{
    for (auto &[name, val] : parameterValues)
    {
        auto it = componentMap.find(name);

        if (it == componentMap.end() || !it->second)
        {
            continue;
        }

        auto *comp = it->second.get();

        if (auto *k = dynamic_cast<Knob *>(comp))
        {
            k->setValue(val, juce::dontSendNotification);
        }
        else if (auto *ms = dynamic_cast<MultiStateButton *>(comp))
        {
            ms->setValue(val, juce::dontSendNotification);
        }
        else if (auto *b = dynamic_cast<ToggleButton *>(comp))
        {
            b->setToggleState(val > 0.5f, juce::dontSendNotification);
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

void ObxfAudioProcessorEditor::clean()
{
    this->removeAllChildren();

    if (aboutScreen)
    {
        addChildComponent(*aboutScreen);
    }
    if (saveDialog)
    {
        addChildComponent(*saveDialog);
    }
    if (mpeMatrixEditor)
    {
        addChildComponent(*mpeMatrixEditor);
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

// Widget creation from XML
void ObxfAudioProcessorEditor::createComponentsFromXml(const juce::XmlElement *doc)
{
    createParameterBoundWidgets(doc);
    createSpecialWidgets(doc);
    cacheLayoutFromXml(doc);

    panelGroups = panelGroupDefinitions();

    for (auto &[name, group] : panelGroups)
    {
        group.resolve(componentMap);
    }

    // If the selector for the active panel doesn't exist in a particular theme,
    // fall back to panel 0 so widgets aren't left permanently hidden
    auto showPanelWithFallback = [&](PanelGroup &group, auto &state) -> int {
        const auto requestedPanel = static_cast<int>(state);
        const auto &selector = group.panels[requestedPanel].selectorWidget;
        const bool selectorExists = !selector.empty() && componentMap.count(selector);
        const int actualPanel = selectorExists ? requestedPanel : 0;

        if (actualPanel != requestedPanel)
        {
            state = static_cast<std::decay_t<decltype(state)>>(actualPanel);
        }

        group.showPanel(actualPanel);

        return actualPanel;
    };

    showPanelWithFallback(panelGroups["master"], processor.selectedMTSESPPanel);

    if (auto *mtsPanel = panelGroups["master"].findPanel("mtsSettingsButton"))
    {
        if (auto *b = getWidget<ToggleButton>(mtsPanel->selectorWidget))
        {
            b->setToggleState(processor.selectedMTSESPPanel, juce::sendNotification);
        }
    }

    showPanelWithFallback(panelGroups["global"], processor.selectedMPEPanel);

    if (auto *mpePanel = panelGroups["global"].findPanel("mpeSettingsButton"))
    {
        if (auto *b = getWidget<ToggleButton>(mpePanel->selectorWidget))
        {
            b->setToggleState(processor.selectedMPEPanel, juce::sendNotification);
        }

        if (mpePanel->childGroup)
        {
            const int mpePanel = showPanelWithFallback(
                *panelGroups["global"].findPanel("mpeSettingsButton")->childGroup,
                processor.selectedMPEDimension);

            // set initial toggle states for MPE dimension select buttons
            if (auto *b = getWidget<ToggleButton>(panelGroups["global"]
                                                      .findPanel("mpeSettingsButton")
                                                      ->childGroup->panels[mpePanel]
                                                      .selectorWidget))
            {
                b->setToggleState(true, juce::sendNotification);
            }
        }
    }

    // set initial toggle states for LFO select buttons
    const int lfoPanel = showPanelWithFallback(panelGroups["lfo"], processor.selectedLFOIndex);

    if (auto *b = getWidget<ToggleButton>(panelGroups["lfo"].panels[lfoPanel].selectorWidget))
    {
        b->setToggleState(true, juce::sendNotification);
    }

    auto switchPanel = [this](PanelGroup &group, int index, std::function<void()> existingClickCb) {
        group.showPanel(index);

        if (existingClickCb)
        {
            existingClickCb();
        }

        if (auto *mlb = getWidget<ToggleButton>("midiLearnButton"); mlb && mlb->getToggleState())
        {
            enterMidiLearnMode();
        }
    };

    updateFilterVisibility = [this](std::function<void()> existingClickCb = nullptr) {
        const bool fourPole = getWidget<ToggleButton>("filter4PoleModeButton") &&
                              getWidget<ToggleButton>("filter4PoleModeButton")->getToggleState();
        const bool xpander = getWidget<ToggleButton>("filter4PoleXpanderButton") &&
                             getWidget<ToggleButton>("filter4PoleXpanderButton")->getToggleState();

        if (existingClickCb)
        {
            existingClickCb();
        }

        if (auto *b = getWidget<ToggleButton>("filter2PoleBPBlendButton"))
            b->setVisible(!fourPole);
        if (auto *b = getWidget<ToggleButton>("filter2PolePushButton"))
            b->setVisible(!fourPole);
        if (auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton"))
            b->setVisible(fourPole);
        if (auto *k = getWidget<Knob>("filterModeKnob"))
            k->setVisible(!(fourPole && xpander));
        if (auto *m = getWidget<ButtonList>("filterXpanderModeMenu"))
            m->setVisible(fourPole && xpander);
    };

    for (auto &[groupName, group] : panelGroups)
    {
        for (int i = 0; i < (int)group.panels.size(); i++)
        {
            if (group.panels[i].selectorWidget.empty())
            {
                continue;
            }

            if (auto *b = getWidget<ToggleButton>(group.panels[i].selectorWidget))
            {
                auto existingClickCb = b->onClick;

                if (b->getRadioGroupId() != 0)
                    b->onClick = [&group, i, existingClickCb, switchPanel]() {
                        switchPanel(group, i, existingClickCb);
                    };
                else
                    b->onClick = [&group, b, existingClickCb, switchPanel]() {
                        switchPanel(group, b->getToggleState() ? 1 : 0, existingClickCb);
                    };
            }
        }
    }

    if (auto *b = getWidget<ToggleButton>("filter4PoleModeButton"))
    {
        auto existingClickCb = b->onClick;
        b->onClick = [this, existingClickCb]() { updateFilterVisibility(existingClickCb); };
    }

    if (auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton"))
    {
        auto existingClickCb = b->onClick;
        b->onClick = [this, existingClickCb]() { updateFilterVisibility(existingClickCb); };
    }

    componentByParamID.clear();

    for (auto &[n, c] : componentMap)
    {
        if (auto *dcp = dynamic_cast<HasParameterWithID *>(c.get()))
        {
            if (dcp->getParameterWithID())
            {
                componentByParamID[dcp->getParameterWithID()->getParameterID()] = c.get();
            }
        }
    }

    if (updateFilterVisibility)
    {
        updateFilterVisibility(nullptr);
    }
}

void ObxfAudioProcessorEditor::createParameterBoundWidgets(const juce::XmlElement *doc)
{
    const auto &knobs = knobRegistry();
    const auto &buttons = buttonRegistry();
    const auto &multiState = multiStateRegistry();
    const auto &lists = listRegistry();

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        const std::string nameStd = child->getStringAttribute("name").toStdString();
        const juce::String name = child->getStringAttribute("name");
        const auto x = child->getIntAttribute("x");
        const auto y = child->getIntAttribute("y");
        const auto w = child->getIntAttribute("w");
        const auto h = child->getIntAttribute("h");
        const auto d = child->getIntAttribute("d");
        const auto fh = child->getIntAttribute("fh");
        const auto pic = child->getStringAttribute("pic");

        if (auto it = knobs.find(nameStd); it != knobs.end())
        {
            const auto &desc = it->second;
            auto knob = addKnob(x, y, w, h, d, fh, juce::String(desc.paramId), desc.defaultVal,
                                juce::String(desc.displayName),
                                useAssetOrDefault(pic, juce::String(desc.defaultAsset)));
            if (!knob)
            {
                continue;
            }

            if (desc.postCreate)
            {
                desc.postCreate(knob.get());
            }

            storeWidget(componentMap, this, name, std::move(knob));

            continue;
        }

        if (auto it = buttons.find(nameStd); it != buttons.end())
        {
            const auto &desc = it->second;
            auto button =
                addButton(x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                          useAssetOrDefault(pic, juce::String(desc.defaultAsset)));

            if (!button)
            {
                continue;
            }

            if (desc.postCreate)
            {
                desc.postCreate(button.get());
            }

            storeWidget(componentMap, this, name, std::move(button));

            continue;
        }

        if (auto it = multiState.find(nameStd); it != multiState.end())
        {
            const auto &desc = it->second;
            auto btn = addMultiStateButton(
                x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                useAssetOrDefault(pic, juce::String(desc.defaultAsset)), desc.numStates);
            if (!btn)
            {
                continue;
            }

            storeWidget(componentMap, this, name, std::move(btn));

            continue;
        }

        if (auto it = lists.find(nameStd); it != lists.end())
        {
            const auto &desc = it->second;
            auto list =
                addList(x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                        useAssetOrDefault(pic, juce::String(desc.defaultAsset)));
            if (!list)
            {
                continue;
            }

            storeWidget(componentMap, this, name, std::move(list));

            continue;
        }
    }
}

void ObxfAudioProcessorEditor::createSpecialWidgets(const juce::XmlElement *doc)
{
    using namespace SynthParam;

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        const juce::String name = child->getStringAttribute("name");
        const auto x = child->getIntAttribute("x");
        const auto y = child->getIntAttribute("y");
        const auto w = child->getIntAttribute("w");
        const auto h = child->getIntAttribute("h");
        const auto pic = child->getStringAttribute("pic");
        const auto color =
            juce::Colour(child->getStringAttribute("color", "FFFF0000").getHexValue32());

        // Skip anything already handled by the parameter-bound pass
        if (componentMap.count(name))
        {
            continue;
        }

        if (name == "patchNameLabel")
        {
            auto label =
                std::make_unique<Display>("Patch Name", [this]() { return impliedScaleFactor(); });

            label->setBounds(transformBounds(x, y, w, h));
            label->setJustificationType(juce::Justification::centred);
            label->setMinimumHorizontalScale(1.f);
            label->setFont(patchNameFont.withHeight(18));
            label->setColour(juce::Label::textColourId, color);
            label->setColour(juce::Label::textWhenEditingColourId, color);
            label->setColour(juce::Label::outlineWhenEditingColourId,
                             juce::Colours::transparentBlack);
            label->setColour(juce::TextEditor::textColourId, color);
            label->setColour(juce::TextEditor::highlightedTextColourId, color);
            label->setColour(juce::TextEditor::highlightColourId, juce::Colour(0x20FFFFFF));
            label->setColour(juce::CaretComponent::caretColourId, color);
            lookAndFeelPtr->textInputColour = color;

            auto *raw = storeWidget(componentMap, this, name, std::move(label));
            auto *display = static_cast<Display *>(raw);

            display->onTextChange = [this, display]() {
                processor.getActiveProgram().setName(display->getText());
            };

            continue;
        }

        if (name == "mtsStatusLabel")
        {
            auto label = std::make_unique<Display>("MTS-ESP Settings",
                                                   [this]() { return impliedScaleFactor(); });

            label->setBounds(transformBounds(x, y, w, h));
            label->setJustificationType(juce::Justification::centred);
            label->setMinimumHorizontalScale(1.f);
            label->setEditable(false);
            label->setFont(patchNameFont.withHeight(18));
            label->setColour(juce::Label::textColourId, color);
            label->setColour(juce::TextEditor::textColourId, color);
            lookAndFeelPtr->textInputColour = color;

            storeWidget(componentMap, this, name, std::move(label));

            continue;
        }

        if (name.startsWith("voice") && name.endsWith("LED"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= MAX_VOICES)
            {
                continue;
            }

            const auto assetName =
                useAssetOrDefault(pic, fmt::format("label-led{}", which / MAX_PANNINGS));

            if (auto bg = addLabel(x, y, w, h, h, fmt::format("Voice {} BG", which), assetName))
            {
                storeWidget(componentMap, this, name.replace("LED", "BG"), std::move(bg));
            }

            if (auto led = addLabel(x, y, w, h, h, fmt::format("Voice {} LED", which), assetName))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(led));
                static_cast<Label *>(raw)->setCurrentFrame(1);
            }

            continue;
        }

        // clang-format off
        struct
        {
            std::string widgetName;
            std::string displayName;
            std::string asset;
            bool toBack;
            bool noMouseInteraction;
        } labelDefs[] = {
            {"masterBGLabel",      "Master Section Background",  "label-bg-master",      true,  true },
            {"globalBGLabel",      "Global Section Background",  "label-bg-global",      true,  true },
            {"mpeLinesLabel",      "MPE Dimension Select Label", "label-mpe-lines",      false, true },
            {"filterModeLabel",    "Filter Mode Label",          "label-filter-mode",    true,  false},
            {"filterOptionsLabel", "Filter Options Label",       "label-filter-options", true,  false},
            {"osc1TriangleLabel",  "Osc 1 Triangle Icon",        "label-osc-triangle",   false, false},
            {"osc1PulseLabel",     "Osc 1 Pulse Icon",           "label-osc-pulse",      false, false},
            {"osc2TriangleLabel",  "Osc 2 Triangle Icon",        "label-osc-triangle",   false, false},
            {"osc2PulseLabel",     "Osc 2 Pulse Icon",           "label-osc-pulse",      false, false},
            {"lfo1Wave2Label",     "LFO 1 Wave 2 Icons",         "label-lfo-wave2",      true,  false},
            {"lfo2Wave2Label",     "LFO 2 Wave 2 Icons",         "label-lfo-wave2",      true,  false},
        };
        // clang-format on

        bool handled = false;

        for (auto &def : labelDefs)
        {
            if (name.compare(def.widgetName) != 0)
            {
                continue;
            }

            if (auto lbl = addLabel(x, y, w, h, h, def.displayName, def.asset))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(lbl));

                if (def.toBack)
                {
                    raw->toBack();
                }

                if (def.noMouseInteraction)
                {
                    raw->setInterceptsMouseClicks(false, false);
                }
            }

            handled = true;

            break;
        }

        if (handled)
        {
            continue;
        }

        if (name == "mtsSettingsButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MTSESPSettings,
                                 useAssetOrDefault(pic, "button-tuning"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->onClick = [this, tb]() {
                processor.selectedMTSESPPanel = tb->getToggleState();

                auto *masterBGLabel = getWidget<Label>("masterBGLabel");
                masterBGLabel->setCurrentFrame(processor.selectedMTSESPPanel);
            };
        }

        if (name == "mainMenu")
        {
            if (auto menu = addMenu(x, y, w, h, useAssetOrDefault(pic, "button-clear-red")))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(menu));

                raw->setTitle("Main Menu");

                juce::Timer::callAfterDelay(30, [w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        w->keyboardFocusMainMenu();
                    }
                });
            }

            continue;
        }

        if (name == "midiLearnButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MidiLearn,
                                 useAssetOrDefault(pic, "button"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->onStateChange = [this, tb]() {
                const bool state = tb->getToggleState();
                paramCoordinator.midiLearnAttachment.set(state);

                if (state)
                {
                    enterMidiLearnMode();
                }
                else
                {
                    exitMidiLearnMode();
                }
            };

            continue;
        }

        if (name == "mpeButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MPEMode,
                                 useAssetOrDefault(pic, "button"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.getMidiHandler().mpeEnabled.load(),
                               juce::dontSendNotification);

            tb->onClick = [this]() {
                const auto mpeStatus = processor.getSynth().getMotherboard()->mpeEnabled;
                processor.setMpeEnabled(!mpeStatus);
            };

            continue;
        }

        if (name == "mpeSettingsButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MPESettings,
                                 useAssetOrDefault(pic, "button-cogwheel"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->onClick = [this, tb]() {
                processor.selectedMPEPanel = tb->getToggleState();

                auto *globalBGLabel = getWidget<Label>("globalBGLabel");
                globalBGLabel->setCurrentFrame(processor.selectedMPEPanel);
            };
        }

        struct MPEDimButton
        {
            std::string name;
            std::string displayName;
            int labelFrame;
        };
        static const MPEDimButton mpeDimButtons[] = {
            {"mpeStrikeSelectButton", Name::MPEStrikeSelect, 0},
            {"mpeLiftSelectButton", Name::MPELiftSelect, 1},
            {"mpePressSelectButton", Name::MPEPressSelect, 2},
            {"mpeSlideSelectButton", Name::MPESlideSelect, 3},
        };

        for (auto &def : mpeDimButtons)
        {
            if (name.compare(def.name) != 0)
            {
                continue;
            }

            auto btn = addButton(x, y, w, h, juce::String{}, def.displayName,
                                 useAssetOrDefault(pic, "button-slim"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->setRadioGroupId(2);

            tb->onClick = [this, frame = def.labelFrame]() {
                processor.selectedMPEDimension = frame;

                if (auto *lbl = getWidget<Label>("mpeLinesLabel"))
                {
                    lbl->setCurrentFrame(frame);
                }

                if (auto *mpePanel = panelGroups["global"].findPanel("mpeSettingsButton"))
                {
                    if (mpePanel->childGroup)
                    {
                        mpePanel->childGroup->showPanel(processor.selectedMPEDimension);
                    }
                }
            };
        }

        if (name == "mpeGlideRangeMenu")
        {
            auto btn = addList(x, y, w, h, juce::String{}, Name::MPEGlideRange, "menu-pitch-bend");
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ButtonList *>(raw);

            tb->onChange = [this, tb]() {
                const auto val = tb->getSelectedItemIndex();

                if (val > -1)
                {
                    processor.setMpePitchBendRange(val);
                }
            };
        }

        if (name == "mtsDynamicButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MTSESPDynamic,
                                 useAssetOrDefault(pic, "button"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.dynamicMTSESP.load(), juce::dontSendNotification);

            continue;
        }

        if (name == "lockHQButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::LockHQ,
                                 useAssetOrDefault(pic, "button-lock"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.lockHighQuality.load(), juce::dontSendNotification);

            tb->onClick = [this, tb]() {
                const bool locked = tb->getToggleState();
                auto &ph = processor.getParamCoordinator().getParameterUpdateHandler();
                const auto hq = ph.getParameter(ID::HQMode);

                processor.lockHighQuality.store(locked);

                if (locked)
                {
                    processor.lockedHQ = hq->convertFrom0to1(hq->getValue());
                }
            };

            continue;
        }

        if (name == "lockBendRangeButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::LockPitchBend,
                                 useAssetOrDefault(pic, "button-lock"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.lockPitchBend.load(), juce::dontSendNotification);

            tb->onClick = [this, tb]() {
                const bool locked = tb->getToggleState();
                auto &ph = processor.getParamCoordinator().getParameterUpdateHandler();
                const auto pbDown = ph.getParameter(ID::BendDownRange);
                const auto pbUp = ph.getParameter(ID::BendUpRange);

                processor.lockPitchBend.store(locked);

                if (locked)
                {
                    processor.lockedPBDownRange =
                        static_cast<int>(pbDown->convertFrom0to1(pbDown->getValue()));
                    processor.lockedPBUpRange =
                        static_cast<int>(pbUp->convertFrom0to1(pbUp->getValue()));
                }
            };

            continue;
        }

        if (name == "patchNumberMenu")
        {
            auto digits = std::make_unique<DisplayDigits>("menu-patch", h, imageCache);
            auto *raw = storeWidget(componentMap, this, name, std::move(digits));
            auto *dd = static_cast<DisplayDigits *>(raw);

            raw->setBounds(transformBounds(x, y, w, h));

            dd->onClick = [this]() {
                juce::PopupMenu m;
                createPatchList(m);
                m.showMenuAsync(obxf::defaultPopupMenuOptions(this), [this](int i) {
                    if (i)
                        MenuActionCallback(i);
                });
            };

            continue;
        }

        if (name == "prevPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::PrevPatch,
                                 useAssetOrDefault(pic, "button-clear"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { prevProgram(); };

            continue;
        }

        if (name == "nextPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::NextPatch,
                                 useAssetOrDefault(pic, "button-clear"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { nextProgram(); };

            continue;
        }

        if (name == "savePatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::SavePatch,
                                 useAssetOrDefault(pic, "button-clear-red"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() {
                if (const auto peer = getPeer())
                {
                    const auto mod = peer->getCurrentModifiersRealtime();
                    MenuActionCallback(mod.isCommandDown() ? MenuAction::QuickSavePatch
                                                           : MenuAction::SavePatch);
                }
                else
                {
                    MenuActionCallback(MenuAction::SavePatch);
                }
            };

            continue;
        }

        if (name == "undoPatchButton")
        {
            auto btn =
                addButton(x, y, w, h, juce::String{}, Name::Undo, useAssetOrDefault(pic, "button"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() {
                processor.getParamCoordinator().getParameterUpdateHandler().undo();
            };

            continue;
        }

        if (name == "initPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::InitializePatch,
                                 useAssetOrDefault(pic, "button-clear-red"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { MenuActionCallback(MenuAction::InitializePatch); };

            continue;
        }

        if (name == "randomizePatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::RandomizePatch,
                                 useAssetOrDefault(pic, "button-clear-white"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { mutateCallback(); };

            tb->onRightClick([this]() { showMutatorMenu(); });

            continue;
        }

        if (name == "groupSelectButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::PatchGroupSelect,
                                 useAssetOrDefault(pic, "button-alt"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onStateChange = [this]() { updateSelectButtonStates(); };

            continue;
        }

        if (name.startsWith("select") && name.endsWith("Button"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= NUM_PATCHES_PER_GROUP)
            {
                continue;
            }

            const juce::String labelName = juce::String(name).replace("Button", "Label");

            if (auto lbl =
                    addLabel(x, y, w, h, h, fmt::format("Select Group/Patch {} Label", which),
                             "button-group-patch"))
            {
                storeWidget(componentMap, this, labelName, std::move(lbl));
            }

            auto btn = addButton(x, y, w, h, juce::String{},
                                 fmt::format("Select Group/Patch {}", which), "");
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->setTriggeredOnMouseDown(true);
            tb->setClickingTogglesState(false);

            tb->onStateChange = [this, whichIdx]() {
                if (auto *b = getWidget<ToggleButton>(fmt::format("select{}Button", whichIdx + 1));
                    b && b->isDown())
                {
                    loadPatchFromProgrammer(whichIdx);
                }
                updateSelectButtonStates();
            };

            continue;
        }

        if (name.startsWith("lfo") && name.endsWith("SelectButton"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= NUM_LFOS)
            {
                continue;
            }

            auto btn = addButton(x, y, w, h, juce::String{}, fmt::format("Select LFO {}", which),
                                 useAssetOrDefault(pic, "button-slim"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->setRadioGroupId(1);

            tb->onClick = [this, whichIdx]() { processor.selectedLFOIndex = whichIdx; };

            continue;
        }

        if (name == "aboutPage")
        {
            auto btn =
                addButton(x, y, w, h, juce::String{}, "About Page", useAssetOrDefault(pic, ""));
            if (btn)
            {
                auto *tb = static_cast<ToggleButton *>(
                    storeWidget(componentMap, this, name, std::move(btn)));

                tb->setWantsKeyboardFocus(false);
                tb->setAccessible(false);

                tb->onClick = [this]() { aboutScreen->showOver(this); };
            }

            continue;
        }

        if (name.startsWith("savePatch") && saveDialog)
        {
            saveDialog->boundsMap[name.toStdString()] = juce::Rectangle<int>(x, y, w, h);

            if (child->hasAttribute("color"))
            {
                saveDialog->colorMap[name.toStdString()] =
                    juce::Colour(child->getStringAttribute("color").getHexValue32());
            }
        }
    }
}

void ObxfAudioProcessorEditor::cacheLayoutFromXml(const juce::XmlElement *doc)
{
    cachedLayout.clear();

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        cachedLayout.push_back({child->getStringAttribute("name"), child->getIntAttribute("x"),
                                child->getIntAttribute("y"), child->getIntAttribute("w"),
                                child->getIntAttribute("h"), child->getIntAttribute("d")});
    }
}

// Low-level widget factories
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
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            knob->setParameter(param);
            knob->setValue(param->getValue(), juce::dontSendNotification);
            knobAttachments.emplace_back(
                new KnobAttachment(paramCoordinator.getParameterUpdateHandler(), param, *knob));
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
        if (auto *param = paramCoordinator.getParameter(paramId))
        {
            button->setParameter(param);
            button->setToggleState(param->getValue() > 0.5f, juce::dontSendNotification);
            toggleAttachments.emplace_back(new ButtonAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *button,
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
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            button->setOptionalParameter(param);
            button->setValue(param->getValue(), juce::dontSendNotification);
            multiStateAttachments.emplace_back(new MultiStateAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *button));
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
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            list->setParameter(param);
            list->setValue(param->getValue(), juce::dontSendNotification);

            buttonListAttachments.emplace_back(new ButtonListAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *list));
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
        {
            return;
        }

        if (auto *mm = safeThis->getWidget<ImageMenu>("mainMenu"))
        {
            auto x = mm->getScreenX();
            auto y = mm->getScreenY();
            auto dx = mm->getWidth();
            auto pos = juce::Point<int>(x, y + dx);
            safeThis->resultFromMenu(pos);
        }
    };

    addAndMakeVisible(menu);

    return std::unique_ptr<ImageMenu>(menu);
}

// Helpers
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