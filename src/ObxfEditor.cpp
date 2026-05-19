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

#include "ObxfProcessor.h"
#include "ObxfEditor.h"
#include "Utils.h"

#include "components/ScalingImageCache.h"

#include "gui/AboutScreen.h"
#include "gui/MPEMatrix.h"
#include "gui/SaveDialog.h"

#include "gui/FocusDebugger.h"
#include "gui/FocusOrder.h"

#include "sst/plugininfra/misc_platform.h"

struct IdleTimer : juce::Timer
{
    ObxfAudioProcessorEditor *editor{nullptr};
    IdleTimer(ObxfAudioProcessorEditor *e) : editor(e) {}
    void timerCallback() override
    {
        if (editor)
            editor->idle();
    }
};

ObxfAudioProcessorEditor::ObxfAudioProcessorEditor(ObxfAudioProcessor &p)
    : AudioProcessorEditor(&p), midiStart(5000), sizeStart(4000), themeStart(1000), processor(p),
      utils(p.getUtils()), paramCoordinator(p.getParamCoordinator()), imageCache(utils),
      themes(utils.getThemeLocations())
{
    skinLoaded = false;
    updateProcessorImpliedScaleFactor = false;

    lookAndFeelPtr = std::make_unique<obxf::LookAndFeel>(this);
    setLookAndFeel(lookAndFeelPtr.get());

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        // We are by definition one process so it is safe to
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeelPtr.get());
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

    saveDialog = std::make_unique<SaveDialog>(*this);
    addChildComponent(*saveDialog);

    mpeMatrixEditor = std::make_unique<MPEMatrixEditor>(processor);
    addChildComponent(*mpeMatrixEditor);

    const auto jersey = juce::Typeface::createSystemTypefaceFor(BinaryData::Jersey20_ttf,
                                                                BinaryData::Jersey20_ttfSize);
    const auto trek =
        juce::Typeface::createSystemTypefaceFor(BinaryData::Trek_ttf, BinaryData::Trek_ttfSize);

    patchNameFont = juce::FontOptions(jersey);
    midiLearnPopupFont = juce::FontOptions(trek);

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

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    inspector = std::make_unique<melatonin::Inspector>(*this, false);
    inspector->setVisible(false);
#endif

    focusDebugger = std::make_unique<FocusDebugger>(*this);
    // focusDebugger->setDoFocusDebug(true);

    auto sf = utils.getDefaultZoomFactor();

    if (processor.lastImpliedScaleFactor != 1.f)
    {
        sf = processor.lastImpliedScaleFactor;
    }

    setSize(juce::roundToInt(static_cast<float>(initialWidth) * sf),
            juce::roundToInt(static_cast<float>(initialHeight) * sf));

    updateProcessorImpliedScaleFactor = true;

    resized();

    for (auto &[n, c] : componentMap)
    {
        if (auto hcf = dynamic_cast<HasScaleFactor *>(c.get()))
        {
            hcf->scaleFactorChanged();
        }
    }

    resizeOnNextIdle = 3;
    ignoreHostScale = false;
    dontParentMenusToEditor = false;

#if JUCE_LINUX
    if (juce::PluginHostType().isBitwigStudio())
        dontParentMenusToEditor = true;
#endif
}

void ObxfAudioProcessorEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    if (utils.getUseSoftwareRenderer())
    {
        if (auto peer = getPeer())
        {
            peer->setCurrentRenderingEngine(0); // 0 for software mode, 1 for Direct2D mode
        }
    }
#endif

    if (isShowing() && isVisible())
    {
        resized();
    }
}

void ObxfAudioProcessorEditor::resized()
{
    scaleFactorChanged();
    themeLocation = utils.getCurrentThemeLocation();

    if (cachedLayout.empty())
    {
        return;
    }

    static FocusOrder focusOrder;

    if (saveDialog)
    {
        saveDialog->resetState();
    }

    for (const auto &layout : cachedLayout)
    {
        const auto &name = layout.name;
        auto it = componentMap.find(name);

        if (it == componentMap.end() || !it->second)
        {
            continue;
        }

        Component *comp = it->second.get();

        auto addOrder = [&]() {
            auto tfo = focusOrder.getOrder(name.toStdString());

            if (tfo != 0 && comp->getTitle().isNotEmpty())
            {
                comp->setExplicitFocusOrder(tfo);
                comp->setWantsKeyboardFocus(true);
            }
        };

        if (auto *knob = dynamic_cast<Knob *>(comp))
        {
            if (layout.d > 0)
                knob->setBounds(transformBounds(layout.x, layout.y, layout.d, layout.d));
            else if (layout.w > 0 && layout.h > 0)
                knob->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            else
                knob->setBounds(
                    transformBounds(layout.x, layout.y, defKnobDiameter, defKnobDiameter));

            knob->setPopupDisplayEnabled(true, true, knob->getParentComponent());
            addOrder();
        }
        else if (dynamic_cast<ButtonList *>(comp) || dynamic_cast<ImageMenu *>(comp) ||
                 dynamic_cast<MultiStateButton *>(comp))
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            addOrder();
        }
        else if (dynamic_cast<ToggleButton *>(comp))
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            addOrder();

            // Programmer select buttons have a paired label at the same bounds
            if (name.startsWith("select") && name.endsWith("Button"))
            {
                auto labelName = juce::String(name).replace("Button", "Label");
                if (auto lit = componentMap.find(labelName);
                    lit != componentMap.end() && lit->second)
                {
                    lit->second->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
                }
            }
        }
        else
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));

            if (name.startsWith("voice") && name.endsWith("LED"))
            {
                auto bgName = juce::String(name).replace("LED", "BG");

                if (auto bgIt = componentMap.find(bgName);
                    bgIt != componentMap.end() && bgIt->second)
                {
                    bgIt->second->setBounds(
                        transformBounds(layout.x, layout.y, layout.w, layout.h));
                }
            }

            addOrder();
        }
    }

    if (saveDialog)
    {
        saveDialog->resized();
    }

    const float sf = impliedScaleFactor();

    for (auto &overlay : midiLearnOverlays)
    {
        if (overlay)
        {
            overlay->setScaleFactor(sf);
        }
    }

    if (aboutScreen)
    {
        aboutScreen->setBounds(getBounds());
    }

    if (saveDialog)
    {
        saveDialog->setBounds(getBounds());
    }

    if (mpeMatrixEditor)
    {
        const int w = MPEMatrixEditor::preferredWidth();
        const int h = MPEMatrixEditor::preferredHeight();

        mpeMatrixEditor->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w, h);
    }

    if (updateProcessorImpliedScaleFactor)
    {
        processor.lastImpliedScaleFactor = impliedScaleFactor();
    }
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
            for (auto &[n, c] : componentMap)
                if (auto hcf = dynamic_cast<HasScaleFactor *>(c.get()))
                    hcf->scaleFactorChanged();
        }
    }

    if (countTimer == 4)
    {
        if (componentMap.count("patchNumberMenu"))
        {
            updatePatchNumberIfNeeded();
        }

        updateSelectButtonStates();
        countTimer = 0;
    }

    // MIDI learn state sync
    if (auto *midiLearnButton = getWidget<ToggleButton>("midiLearnButton"))
    {
        const auto mla = paramCoordinator.midiLearnAttachment.get();
        const auto mts = midiLearnButton->getToggleState();

        if (mla != mts)
        {
            if (!mla)
            {
                exitMidiLearnMode();
            }

            midiLearnButton->setToggleState(mla, juce::dontSendNotification);
        }

        const auto lup = processor.getMidiHandler().getLastUsedParameter();

        if (lup > 0)
        {
            for (auto &p : ParameterList)
            {
                if (p.meta.id == (uint32_t)lup && p.ID != midiLearnLastUsedPID)
                {
                    midiLearnLastUsedPID = p.ID;
                    repaint();
                }
            }
        }
        else if (midiLearnLastUsedPID.isNotEmpty())
        {
            midiLearnLastUsedPID = "";
            repaint();
        }
    }

    // Voice LEDs
    if (auto *polyphonyMenu = getWidget<ButtonList>("polyphonyMenu"))
    {
        const int curPoly = juce::jmin(polyphonyMenu->getSelectedId(), MAX_VOICES);

        for (int i = 0; i < MAX_VOICES; i++)
        {
            const auto ledName = fmt::format("voice{}LED", i + 1);
            const auto bgName = fmt::format("voice{}BG", i + 1);
            auto *led = getWidget<Label>(ledName);
            auto *bg = getWidget<Label>(bgName);

            if (!led || !bg)
            {
                continue;
            }

            if (i >= curPoly && bg->isVisible())
            {
                bg->setVisible(false);
                led->setVisible(false);
            }
            else if (i < curPoly && !bg->isVisible())
            {
                bg->setVisible(true);
                led->setVisible(true);
            }
        }

        for (int i = 0; i < curPoly; i++)
        {
            const auto state =
                std::cbrtf(static_cast<float>(processor.uiState.voiceStatusValue[i]));
            auto *led = getWidget<Label>(fmt::format("voice{}LED", i + 1));

            if (led && state != led->getAlpha())
            {
                led->setAlpha(state);
            }
        }

        for (int i = curPoly; i < MAX_VOICES; i++)
        {
            auto *led = getWidget<Label>(fmt::format("voice{}LED", i + 1));

            if (led && led->getAlpha() > 0.f)
            {
                led->setAlpha(0.f);
            }
        }
    }

    // Osc triangle labels
    {
        auto *osc1Saw = getWidget<ToggleButton>("osc1SawButton");
        auto *osc1Pulse = getWidget<ToggleButton>("osc1PulseButton");
        auto *tri1 = getWidget<Label>("osc1TriangleLabel");

        if (tri1 && osc1Saw && osc1Pulse)
        {
            const int state = !(osc1Saw->getToggleState() || osc1Pulse->getToggleState());

            if (tri1->getCurrentFrame() != state)
            {
                tri1->setCurrentFrame(state);
            }
        }
    }
    {
        auto *osc2Saw = getWidget<ToggleButton>("osc2SawButton");
        auto *osc2Pulse = getWidget<ToggleButton>("osc2PulseButton");
        auto *tri2 = getWidget<Label>("osc2TriangleLabel");

        if (tri2 && osc2Saw && osc2Pulse)
        {
            const int state = !(osc2Saw->getToggleState() || osc2Pulse->getToggleState());

            if (tri2->getCurrentFrame() != state)
            {
                tri2->setCurrentFrame(state);
            }
        }
    }

    // Pulsewidth labels
    {
        auto *pwKnob = getWidget<Knob>("oscPWKnob");
        auto *pw2Knob = getWidget<Knob>("osc2PWOffsetKnob");
        auto *pulse1Label = getWidget<Label>("osc1PulseLabel");
        auto *pulse2Label = getWidget<Label>("osc2PulseLabel");

        if (pulse1Label && pulse2Label && pwKnob && pw2Knob)
        {
            const auto pw1 = juce::roundToInt(pwKnob->getValue() * 46.f);
            const auto pw2 = juce::jmin(pw1 + juce::roundToInt(pw2Knob->getValue() * 46.f), 50);

            if (pulse1Label->getCurrentFrame() != pw1)
            {
                pulse1Label->setCurrentFrame(pw1);
            }

            if (pulse2Label->getCurrentFrame() != pw2)
            {
                pulse2Label->setCurrentFrame(pw2);
            }
        }
    }

    // LFO Wave 2 labels
    {
        auto *lfo1PW = getWidget<Knob>("lfo1PWSlider");
        auto *wave2Lbl1 = getWidget<Label>("lfo1Wave2Label");

        if (wave2Lbl1 && wave2Lbl1->isVisible() && lfo1PW)
        {
            const int state = juce::roundToInt(lfo1PW->getValue() * 24.f);

            if (wave2Lbl1->getCurrentFrame() != state)
            {
                wave2Lbl1->setCurrentFrame(state);
            }
        }
    }
    {
        auto *lfo2PW = getWidget<Knob>("lfo2PWSlider");
        auto *wave2Lbl2 = getWidget<Label>("lfo2Wave2Label");

        if (wave2Lbl2 && wave2Lbl2->isVisible() && lfo2PW)
        {
            const int state = juce::roundToInt(lfo2PW->getValue() * 24.f);

            if (wave2Lbl2->getCurrentFrame() != state)
            {
                wave2Lbl2->setCurrentFrame(state);
            }
        }
    }

    // Filter mode/options labels and button visibility
    {
        const auto fourPole = [&] {
            auto *b = getWidget<ToggleButton>("filter4PoleModeButton");
            return b && b->getToggleState();
        }();
        const auto xpanderMode = [&] {
            auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton");
            return b && b->getToggleState();
        }();
        const auto bpBlend = [&] {
            auto *b = getWidget<ToggleButton>("filter2PoleBPBlendButton");
            return b && b->getToggleState();
        }();

        const int filterModeFrame = fourPole ? (xpanderMode ? 3 : 2) : (bpBlend ? 1 : 0);

        if (auto *filterModeLabel = getWidget<Label>("filterModeLabel"))
        {
            if (filterModeLabel->getCurrentFrame() != filterModeFrame)
            {
                filterModeLabel->setCurrentFrame(filterModeFrame);
            }
        }

        auto *filterOptionsLabel = getWidget<Label>("filterOptionsLabel");

        if (filterOptionsLabel && (int)fourPole != filterOptionsLabel->getCurrentFrame())
        {
            filterOptionsLabel->setCurrentFrame(fourPole);
        }
    }

    // Unison voices menu dimming
    {
        auto *unisonButton = getWidget<ToggleButton>("unisonButton");
        auto *unisonVoicesMenu = getWidget<ButtonList>("unisonVoicesMenu");

        if (unisonButton && unisonVoicesMenu)
        {
            const bool on = unisonButton->getToggleState();

            if (on && unisonVoicesMenu->getAlpha() < 1.f)
            {
                unisonVoicesMenu->setAlpha(1.f);
            }

            if (!on && unisonVoicesMenu->getAlpha() == 1.f)
            {
                unisonVoicesMenu->setAlpha(0.25f);
            }
        }
    }

    // Patch name display
    if (auto *patchNameLabel = getWidget<Display>("patchNameLabel"))
    {
        if (!patchNameLabel->isBeingEdited())
        {
            const auto oldName = processor.getActiveProgram().getName();

            if (patchNameLabel->getText() != oldName)
            {
                patchNameLabel->setText(oldName, juce::dontSendNotification);
            }
        }
    }
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    if (inspector)
    {
        inspector.reset();
    }
#endif

    focusDebugger.reset();

    processor.uiState.editorAttached = false;
    idleTimer->stopTimer();
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    popupMenus.clear();
    componentMap.clear();

    juce::PopupMenu::dismissAllActiveMenus();
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

void ObxfAudioProcessorEditor::actionListenerCallback(const juce::String & /*message*/) {}

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

    updatePatchNumberIfNeeded();
    updateSelectButtonStates();

    if (auto *b = getWidget<ToggleButton>("lockBendRangeButton"))
    {
        b->setToggleState(processor.lockPitchBend.load(), juce::dontSendNotification);
    }

    if (auto *b = getWidget<ToggleButton>("lockHQButton"))
    {
        b->setToggleState(processor.lockHighQuality.load(), juce::dontSendNotification);
    }

    auto &midiHandler = processor.getMidiHandler();

    if (auto *b = getWidget<ToggleButton>("mpeButton"))
    {
        b->setToggleState(midiHandler.mpeEnabled.load(), juce::dontSendNotification);
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

#if !JUCE_IOS
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
    // we will take only the first file from the drop
    if (files.size() > 0)
    {
        const auto file = juce::File(files[0]);
        const juce::String ext = file.getFileExtension().toLowerCase();

        if (ext == ".fxp")
        {
            utils.loadPatch(file);
        }
        else if (ext == ".fxb")
        {
            importObxdBank(file);
        }
    }
}
#endif

float ObxfAudioProcessorEditor::impliedScaleFactor() const
{
    if (initialHeight == 0 || initialWidth == 0)
        return 1.f;

    auto xf = getWidth() / static_cast<float>(initialWidth);
    auto yf = getHeight() / static_cast<float>(initialHeight);

    return std::max(xf, yf);
}

float ObxfAudioProcessorEditor::menuScaleFactor() const
{
    auto ms = utils.getMenuScaleMode();
    switch (ms)
    {
    case Utils::DONT:
        return 1.f;
    case Utils::WITH_OS:
        return utils.getPluginAPIScale();
    case Utils::WITH_PLUGIN:
    {
        auto psf = std::max(1.f, utils.getPluginAPIScale());
        auto rs = impliedScaleFactor() / psf;
        if (rs <= 1)
            return rs;

        return std::sqrt(rs); // slow it down a bit
    }
    }
    return 1.f;
}

void ObxfAudioProcessorEditor::setScaleFactor(float newScale)
{
    if (ignoreHostScale)
        newScale = 1.f;
    utils.setPluginAPIScale(newScale);
    // this line causes the crash with bitmap assets we've been chasing
    // Why? We kinda need it I think...
    // AudioProcessorEditor::setScaleFactor(newScale);
}
