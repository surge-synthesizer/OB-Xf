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

#ifndef OBXF_SRC_PLUGINEDITOR_H
#define OBXF_SRC_PLUGINEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "ObxfProcessor.h"
#include "gui/Knob.h"
#include "gui/ToggleButton.h"
#include "gui/MultiStateButton.h"
#include "gui/ButtonList.h"
#include "gui/ImageMenu.h"
#include "gui/Display.h"
#include "gui/Label.h"
#include "components/ScalingImageCache.h"
#include "Constants.h"
#include "Utils.h"
#include "KeyCommandHandler.h"

#include "gui/LookAndFeel.h"

#include "parameter/ParameterAttachment.h"
#include "parameter/ParameterCoordinator.h"

#include "components/MidiLearnOverlay.h"

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
#include "melatonin_inspector/melatonin_inspector.h"
#endif

struct AboutScreen;
struct SaveDialog;
struct FocusDebugger;

class MPEMatrixEditor;

using KnobAttachment = Attachment<Knob, true>;
using ButtonAttachment = Attachment<ToggleButton, false>;
using ButtonListAttachment = Attachment<ButtonList, false>;
using MultiStateAttachment = Attachment<MultiStateButton, false>;

class ObxfAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       public juce::AsyncUpdater,
                                       public juce::ChangeListener,
                                       public juce::Button::Listener,
                                       public juce::ActionListener
#if !JUCE_IOS
    ,
                                       public juce::FileDragAndDropTarget
#endif
{
  public:
    explicit ObxfAudioProcessorEditor(ObxfAudioProcessor &ownerFilter);

    ~ObxfAudioProcessorEditor() override;

#if !JUCE_IOS
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
#endif

    void scaleFactorChanged();
    void mouseUp(const juce::MouseEvent &e) override;
    void paint(juce::Graphics &g) override;
    void updateFromHost();
    void handleAsyncUpdate() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void buttonClicked(juce::Button *) override {}
    void resized() override;
    int32_t resizeOnNextIdle{-1};
    bool isHighResolutionDisplay() const { return utils.getPixelScaleFactor() > 1.0; }
    void actionListenerCallback(const juce::String &message) override;
    void parentHierarchyChanged() override;
    void setScaleFactor(float newScale) override;
    void idle();

  private:
#include "editor/ObxfEditorLayout.h"
#include "editor/ObxfEditorMenus.h"
#include "editor/ObxfEditorMidiLearn.h"
#include "editor/ObxfEditorProgrammer.h"
#include "editor/ObxfEditorTheme.h"

    std::unique_ptr<juce::Timer> idleTimer;
    std::unique_ptr<juce::XmlElement> cachedThemeXml;

    std::unique_ptr<juce::ComponentBoundsConstrainer> constrainer;
    ObxfAudioProcessor &processor;
    Utils &utils;
    ParameterCoordinator &paramCoordinator;
    std::unique_ptr<KeyCommandHandler> keyCommandHandler;

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    std::unique_ptr<melatonin::Inspector> inspector{};
#endif
    std::unique_ptr<FocusDebugger> focusDebugger;

    bool backgroundIsSVG{false};
    juce::Image backgroundImage;

    std::map<juce::String, std::unique_ptr<juce::Component>> componentMap;
    std::map<juce::String, juce::Component *> componentByParamID;

    ScalingImageCache imageCache;

    Utils::ThemeLocation themeLocation;

    std::vector<std::unique_ptr<KnobAttachment>> knobAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<ButtonListAttachment>> buttonListAttachments;
    std::vector<std::unique_ptr<MultiStateAttachment>> multiStateAttachments;

    bool skinLoaded{false};

    static constexpr int numScaleFactors{10};
    static constexpr float scaleFactors[] = {.75f,  .85f, 1.0f, 1.25f, 1.5f,
                                             1.75f, 2.0f, 2.5f, 3.f,   4.f};
    friend class obxf::LookAndFeel;
    float impliedScaleFactor() const;
    float menuScaleFactor() const;
    bool updateProcessorImpliedScaleFactor{false};

    std::vector<Utils::ThemeLocation> themes;
    std::vector<Utils::MidiLocation> midiFiles;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::FontOptions patchNameFont;
    juce::FontOptions midiLearnPopupFont;
    juce::ApplicationCommandManager commandManager;

    int countTimer{0};
    std::unique_ptr<obxf::LookAndFeel> lookAndFeelPtr;

    juce::Rectangle<int> initialBounds;
    int initialWidth{0};
    int initialHeight{0};

    bool ignoreHostScale{false};
    bool dontParentMenusToEditor{false};

    void initializeEditorCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxfAudioProcessorEditor)
};

#endif // OBXF_SRC_PLUGINEDITOR_H
