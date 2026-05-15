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

// Widget descriptor types for the parameter-bound registries
using KnobPostCreate = std::function<void(Knob *)>;
using ButtonPostCreate = std::function<void(ToggleButton *)>;

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

// Panel group types for tab-switching
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
    Panel *findPanel(const std::string &selectorName);
};

// Cached per-widget layout entry, populated once from XML, reused in resized()
struct WidgetLayout
{
    juce::String name;
    int x{0}, y{0}, w{0}, h{0}, d{0};
};

// Static registries and panel group definitions
static const std::unordered_map<std::string, KnobDescriptor> &knobRegistry();
static const std::unordered_map<std::string, ButtonDescriptor> &buttonRegistry();
static const std::unordered_map<std::string, MultiStateDescriptor> &multiStateRegistry();
static const std::unordered_map<std::string, ListDescriptor> &listRegistry();
static std::map<std::string, PanelGroup> panelGroupDefinitions();

// Members
std::map<std::string, PanelGroup> panelGroups;
std::vector<WidgetLayout> cachedLayout;