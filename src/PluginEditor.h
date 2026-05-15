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

#include "PluginProcessor.h"
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
    static constexpr int defKnobDiameter = 40;

    explicit ObxfAudioProcessorEditor(ObxfAudioProcessor &ownerFilter);

    ~ObxfAudioProcessorEditor() override;

#if !JUCE_IOS
    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
#endif

    void scaleFactorChanged();
    void mouseUp(const juce::MouseEvent &e) override;
    void paint(juce::Graphics &g) override;
    void paintMissingAssets(juce::Graphics &g);
    void updateFromHost();
    void handleAsyncUpdate() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void buttonClicked(juce::Button *) override {}
    void nextProgram();
    void prevProgram();
    void MenuActionCallback(int action);
    void resized() override;
    int32_t resizeOnNextIdle{-1};
    bool isHighResolutionDisplay() const { return utils.getPixelScaleFactor() > 1.0; }
    void actionListenerCallback(const juce::String &message) override;
    void parentHierarchyChanged() override;

  private:
    // Typed map accessor — the primary way to reach any widget after creation.
    // Returns nullptr if the widget doesn't exist or isn't the requested type.
    template <typename T> T *getWidget(const juce::String &name) const
    {
        auto it = componentMap.find(name);
        if (it == componentMap.end())
            return nullptr;
        return dynamic_cast<T *>(it->second.get());
    }

    juce::Rectangle<int> transformBounds(int x, int y, int w, int h) const;

    juce::String useAssetOrDefault(const juce::String &assetName,
                                   const juce::String &defaultAssetName) const;

    std::unique_ptr<Label> addLabel(int x, int y, int w, int h, int fh, const juce::String &name,
                                    const juce::String &assetName);

    std::unique_ptr<Knob> addKnob(int x, int y, int w, int h, int d, int fh,
                                  const juce::String &paramId, float defval,
                                  const juce::String &name, const juce::String &assetName);

    std::unique_ptr<ToggleButton> addButton(int x, int y, int w, int h, const juce::String &paramId,
                                            const juce::String &name,
                                            const juce::String &assetName);

    std::unique_ptr<MultiStateButton> addMultiStateButton(int x, int y, int w, int h,
                                                          const juce::String &paramId,
                                                          const juce::String &name,
                                                          const juce::String &assetName,
                                                          uint8_t numStates = 3);

    std::unique_ptr<ButtonList> addList(int x, int y, int w, int h, const juce::String &paramId,
                                        const juce::String &name, const juce::String &assetName);

    std::unique_ptr<ImageMenu> addMenu(int x, int y, int w, int h, const juce::String &assetName);

    juce::PopupMenu createPatchList(juce::PopupMenu &menu) const;
    int patchesInCurrentFolder() const;

    void enterMidiLearnMode();
    AnchorPosition determineAnchorPosition(Component *comp, const juce::String &paramId);
    void exitMidiLearnMode();

  public:
    void setScaleFactor(float newScale) override;

  private:
    void createMenu();
    void createMidiMapMenu(int, juce::PopupMenu &);
    void resultFromMenu(juce::Point<int>);
    void clean();
    void rebuildComponents(ObxfAudioProcessor &);
    void updateSelectButtonStates();
    void loadTheme(ObxfAudioProcessor &);
    bool loadThemeFilesAndCheck();
    std::map<juce::String, float> saveComponentParameterValues();
    void clearAndResetComponents(ObxfAudioProcessor &ownerFilter);
    bool parseAndCreateComponentsFromTheme();
    void setupMenus();
    void restoreComponentParameterValues(const std::map<juce::String, float> &parameterValues);
    void finalizeThemeLoad(ObxfAudioProcessor &ownerFilter);
    void createComponentsFromXml(const juce::XmlElement *doc);
    void createParameterBoundWidgets(const juce::XmlElement *doc);
    void createSpecialWidgets(const juce::XmlElement *doc);
    void cacheLayoutFromXml(const juce::XmlElement *doc);
    void setupPolyphonyMenu() const;
    void setupUnisonVoicesMenu() const;
    void setupEnvLegatoModeMenu() const;
    void setupNotePriorityMenu() const;
    void setupBendUpRangeMenu() const;
    void setupBendDownRangeMenu() const;
    void setupFilterXpanderModeMenu() const;
    void keyboardFocusMainMenu();
    void showMutatorMenu();
    void mutateCallback();
    void randomizeCallback();

  public:
    void idle();

  private:
    using KnobPostCreate = std::function<void(Knob *)>;
    using ButtonPostCreate = std::function<void(ToggleButton *)>;

    struct PanelGroup;

    struct Panel
    {
        std::string selectorWidget;
        std::vector<std::string> widgetNames;
        std::unique_ptr<PanelGroup> childGroup;
        std::vector<juce::Component *> resolvedWidgets;
    };

    struct PanelGroup
    {
        std::vector<Panel> panels;
        int activePanel{0};

        void resolve(const std::map<juce::String, std::unique_ptr<juce::Component>> &componentMap);
        void showPanel(int index);
        void hideAll();
        Panel *findPanel(const std::string &selectorWidget);
    };

    struct KnobDescriptor
    {
        std::string paramId;
        std::string displayName;
        float defaultVal{0.f};
        std::string defaultAsset{"knob"};
        KnobPostCreate postCreate;
    };

    struct ButtonDescriptor
    {
        std::string paramId;
        std::string displayName;
        std::string defaultAsset{"button"};
        ButtonPostCreate postCreate;
    };

    struct MultiStateDescriptor
    {
        std::string paramId;
        std::string displayName;
        std::string defaultAsset{"button-dual"};
        uint8_t numStates{3};
    };

    struct ListDescriptor
    {
        std::string paramId;
        std::string displayName;
        std::string defaultAsset;
    };

    // Cached per-widget layout entry, populated once from XML and reused in resized()
    struct WidgetLayout
    {
        juce::String name;
        int x{0}, y{0}, w{0}, h{0}, d{0};
    };

    static const std::unordered_map<std::string, KnobDescriptor> &knobRegistry();
    static const std::unordered_map<std::string, ButtonDescriptor> &buttonRegistry();
    static const std::unordered_map<std::string, MultiStateDescriptor> &multiStateRegistry();
    static const std::unordered_map<std::string, ListDescriptor> &listRegistry();
    static std::map<std::string, PanelGroup> panelGroupDefinitions();

    std::map<std::string, PanelGroup> panelGroups;

    std::unique_ptr<juce::Timer> idleTimer;
    std::unique_ptr<juce::XmlElement> cachedThemeXml;
    std::vector<WidgetLayout> cachedLayout;

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

    std::vector<std::unique_ptr<juce::PopupMenu>> popupMenus;

    std::unique_ptr<AboutScreen> aboutScreen;
    friend struct AboutScreen;

    std::unique_ptr<SaveDialog> saveDialog;
    friend struct SaveDialog;

    std::unique_ptr<MPEMatrixEditor> mpeMatrixEditor;

    bool notLoadTheme{false};
    bool skinLoaded{false};
    size_t midiStart{};
    size_t sizeStart{};
    size_t themeStart{};

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

    bool noThemesAvailable{false};
    juce::Rectangle<int> initialBounds;
    int initialWidth{0};
    int initialHeight{0};

    void updatePatchNumberIfNeeded();
    void loadPatchFromProgrammer(int whichButton);
    uint8_t currProgrammerGroup{0}, currProgrammerPatch{0};

    bool ignoreHostScale{false};
    bool dontParentMenusToEditor{false};

    bool midiLearnMode{false};
    juce::String midiLearnLastUsedPID;
    std::vector<std::unique_ptr<MidiLearnOverlay>> midiLearnOverlays;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxfAudioProcessorEditor)
};

#endif // OBXF_SRC_PLUGINEDITOR_H
