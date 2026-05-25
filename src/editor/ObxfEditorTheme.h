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

static constexpr int defKnobDiameter = 40;

// Theme lifecycle
void loadTheme(ObxfAudioProcessor &);
std::map<juce::String, float> saveComponentParameterValues();
void clearAndResetComponents(ObxfAudioProcessor &);
bool parseAndCreateComponentsFromTheme();
void restoreComponentParameterValues(const std::map<juce::String, float> &parameterValues);
void finalizeThemeLoad(ObxfAudioProcessor &ownerFilter);
void clean();
void rebuildComponents(ObxfAudioProcessor &ownerFilter);

// Widget creation from XML
void createComponentsFromXml(const juce::XmlElement *doc);
void createParameterBoundWidgets(const juce::XmlElement *doc);
void createSpecialWidgets(const juce::XmlElement *doc);
void cacheLayoutFromXml(const juce::XmlElement *doc);

template <typename T> T *getWidget(const juce::String &name) const
{
    auto it = componentMap.find(name);
    if (it == componentMap.end())
        return nullptr;
    return dynamic_cast<T *>(it->second.get());
}

template <typename T>
static T *storeWidget(std::map<juce::String, std::unique_ptr<juce::Component>> &map,
                      ObxfAudioProcessorEditor *editor, const juce::String &name,
                      std::unique_ptr<T> widget)
{
    if (!widget)
    {
        return nullptr;
    }

    auto *raw = widget.get();

    map[name] = std::move(widget);
    editor->addAndMakeVisible(*raw);

    return raw;
}

// Low-level widget factories
std::unique_ptr<Label> addLabel(int x, int y, int w, int h, int fh, const juce::String &name,
                                const juce::String &assetName);

std::unique_ptr<Knob> addKnob(int x, int y, int w, int h, int d, int fh,
                              const juce::String &paramId, float defval, const juce::String &name,
                              const juce::String &assetName);

std::unique_ptr<ToggleButton> addButton(int x, int y, int w, int h, const juce::String &paramId,
                                        const juce::String &name, const juce::String &assetName);

std::unique_ptr<MultiStateButton>
addMultiStateButton(int x, int y, int w, int h, const juce::String &paramId,
                    const juce::String &name, const juce::String &assetName, uint8_t numStates = 3);

std::unique_ptr<ButtonList> addList(int x, int y, int w, int h, const juce::String &paramId,
                                    const juce::String &name, const juce::String &assetName);

std::unique_ptr<ImageMenu> addMenu(int x, int y, int w, int h, const juce::String &assetName);

// Helpers
juce::Rectangle<int> transformBounds(int x, int y, int w, int h) const;
juce::String useAssetOrDefault(const juce::String &assetName,
                               const juce::String &defaultAssetName) const;

// Members
std::unique_ptr<AboutScreen> aboutScreen;
friend struct AboutScreen;

std::unique_ptr<SaveDialog> saveDialog;
friend struct SaveDialog;

// std::unique_ptr<MPEMatrixEditor> mpeMatrixEditor;

std::function<void(std::function<void()>)> updateFilterVisibility;