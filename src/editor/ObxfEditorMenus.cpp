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
#include "gui/MutatorMenu.h"
#include "gui/SaveDialog.h"

#include "gui/FocusDebugger.h"
#include "gui/FocusOrder.h"

#include "state/ObxdImporter.h"

// Menu setup
void ObxfAudioProcessorEditor::setupPolyphonyMenu() const
{
    if (auto *m = getWidget<ButtonList>("polyphonyMenu"))
    {
        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(2);

        if (const auto *p = paramCoordinator.getParameter(ID::Polyphony))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupUnisonVoicesMenu() const
{
    if (auto *m = getWidget<ButtonList>("unisonVoicesMenu"))
    {
        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(2);

        if (const auto *p = paramCoordinator.getParameter(ID::UnisonVoices))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupEnvLegatoModeMenu() const
{
    using namespace sst::plugininfra::misc_platform;

    if (auto *m = getWidget<ButtonList>("envLegatoModeMenu"))
    {
        m->addChoice(toOSCase("Both Envelopes"));
        m->addChoice(toOSCase("Filter Envelope Only"));
        m->addChoice(toOSCase("Amplifier Envelope Only"));
        m->addChoice(toOSCase("Always Retrigger"));

        if (const auto *p = paramCoordinator.getParameter(ID::EnvLegatoMode))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupNotePriorityMenu() const
{
    if (auto *m = getWidget<ButtonList>("notePriorityMenu"))
    {
        m->addChoice("Last");
        m->addChoice("Low");
        m->addChoice("High");

        if (const auto *p = paramCoordinator.getParameter(ID::NotePriority))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupBendUpRangeMenu() const
{
    if (auto *m = getWidget<ButtonList>("bendUpRangeMenu"))
    {
        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(4);

        if (const auto *p = paramCoordinator.getParameter(ID::BendUpRange))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupBendDownRangeMenu() const
{
    if (auto *m = getWidget<ButtonList>("bendDownRangeMenu"))
    {
        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(4);

        if (const auto *p = paramCoordinator.getParameter(ID::BendDownRange))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupMPEGlideRangeMenu() const
{
    if (auto *m = getWidget<ButtonList>("mpeGlideRangeMenu"))
    {
        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(4);
        m->setScrollWheelEnabled(true);

        /*         if (const auto *p = paramCoordinator.getParameter(ID::BendDownRange))
                {
                    m->setValue(p->getValue(), juce::dontSendNotification);
                } */
    }
}

void ObxfAudioProcessorEditor::setupFilterXpanderModeMenu() const
{
    if (auto *m = getWidget<ButtonList>("filterXpanderModeMenu"))
    {
        for (const auto *choice : {"LP4", "LP3", "LP2", "LP1", "HP3", "HP2", "HP1", "BP4", "BP2",
                                   "N2", "PH3", "HP2+LP1", "HP3+LP1", "N2+LP1", "PH3+LP1"})
        {
            m->addChoice(choice);
        }

        if (const auto *p = paramCoordinator.getParameter(ID::FilterXpanderMode))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupMPEMatrixMenus() const
{
    using namespace SynthParam;

    for (const auto &def : mpeMatrixWidgetDefs)
    {
        if (auto *m = getWidget<ButtonList>(def.destWidget))
        {
            m->addChoice("None");

            for (const auto &id : matrixCommonTargets())
            {
                m->addChoice(id);
            }

            for (const auto &id : matrixExtraTargets(def.source))
            {
                m->addChoice(id);
            }

            m->setScrollWheelEnabled(true);
        }
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
    setupMPEGlideRangeMenu();
    setupFilterXpanderModeMenu();
    setupMPEMatrixMenus();

    createMenu();
}

// Main menu
void ObxfAudioProcessorEditor::createMenu()
{
    using namespace sst::plugininfra::misc_platform;

    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    themes = utils.getThemeLocations();

    createMidiMapMenu(static_cast<int>(midiStart), midiMenu);

    menu->addSubMenu(toOSCase("MIDI Mapping"), midiMenu);

    /*     menu->addItem(toOSCase("MPE Assignments..."), [w = juce::Component::SafePointer(this)]()
       { if (!w || !w->mpeMatrixEditor) return; w->mpeMatrixEditor->refresh();
            w->mpeMatrixEditor->setVisible(true);
            w->mpeMatrixEditor->toFront(true);
        });

        menu->addSeparator(); */

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
                {
                    themeMenu.addSeparator();
                }

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
            sizeMenu.addSeparator();
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

            sizeMenu.addItem(toOSCase(fmt::format("Zoom to Default Level ({:.{}f}%)",
                                                  utils.getDefaultZoomFactor() * 100.f, 0)),
                             disp, false, [this]() {
                                 auto zf = utils.getDefaultZoomFactor();
                                 const int newWidth =
                                     juce::roundToInt(static_cast<float>(initialWidth) * zf);
                                 const int newHeight =
                                     juce::roundToInt(static_cast<float>(initialHeight) * zf);

                                 setSize(newWidth, newHeight);
                                 resized();
                             });

            sizeMenu.addSeparator();

            sizeMenu.addItem(
                toOSCase(fmt::format("Set {:.{}f}% as Default Zoom Level", dispZoom * 100.f,
                                     isCurZoomAmongScaleFactors ? 0 : 1)),
                disp, false, [this, curZoom]() { utils.setDefaultZoomFactor(curZoom); });

            sizeMenu.addSeparator();

            juce::PopupMenu menuZoomMenu;
            auto ms = utils.getMenuScaleMode();
            menuZoomMenu.addItem("Disabled", true, ms == Utils::MenuScaleMode::DONT,
                                 [w = juce::Component::SafePointer(this)]() {
                                     if (w)
                                         w->utils.setMenuScaleMode(Utils::MenuScaleMode::DONT);
                                 });

            const auto plugin =
                (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                    ? ""
                    : " (Plugin)";
            menuZoomMenu.addItem(
                fmt::format("Enabled{}", plugin), true, ms == Utils::MenuScaleMode::WITH_PLUGIN,
                [w = juce::Component::SafePointer(this)]() {
                    if (w)
                        w->utils.setMenuScaleMode(Utils::MenuScaleMode::WITH_PLUGIN);
                });
            sizeMenu.addSubMenu(toOSCase("Menu Scaling"), menuZoomMenu);

#ifndef JUCE_MAC
            const auto host =
                (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                    ? "Operating System"
                    : "Host";
            menuZoomMenu.addItem(fmt::format("Enabled ({})", host), true,
                                 ms == Utils::MenuScaleMode::WITH_OS,
                                 [w = juce::Component::SafePointer(this)]() {
                                     if (w)
                                         w->utils.setMenuScaleMode(Utils::MenuScaleMode::WITH_OS);
                                 });
#endif
        }

        menu->addSubMenu("Zoom", sizeMenu);
    }

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
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
#if !JUCE_IOS
    menu->addItem(MenuAction::RevealUserDirectory, toOSCase("Open User Data Folder..."), true,
                  false);
#endif
    menu->addItem(toOSCase("Open Manual..."), []() {
        juce::URL("https://surge-synth-team.org/ob-xf/manual/getting-started/")
            .launchInDefaultBrowser();
    });

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

    const juce::File midi_dir = utils.getMidiFolderFor(Utils::LocationType::USER);

    menuMidi.addItem(toOSCase("Clear MIDI Mapping"), true, false,
                     [this]() { processor.getMidiMap().reset(); });

    menuMidi.addItem(toOSCase("Save MIDI Mapping..."), true, false, [this, midi_dir]() {
        fileChooser =
            std::make_unique<juce::FileChooser>("Save MIDI Mapping", midi_dir, "*.xml", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    processor.getMidiHandler().saveBindingsTo(result);
                }
            });
    });

    menuMidi.addSeparator();

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

    popupMenus[0]->showMenuAsync(obxf::defaultPopupMenuOptions(this), [this](size_t result) {
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
    }

    if (action == MenuAction::LoadPatch)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Load Patch", juce::File(), "*.fxp", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    utils.loadPatch(result);
                }
            });
    }

    if (action == MenuAction::SavePatch)
    {
        saveDialog->showOver(this);
    }

    if (action == MenuAction::QuickSavePatch)
    {
        saveDialog->doQuickSave();
    }

    if (action == MenuAction::DeletePatch)
    {
        auto lsp = processor.lastLoadedPatchNode.lock();

        if (lsp)
        {
            auto llp = lsp->index;
            auto &curPatch = utils.patchesAsLinearList[llp];
            const auto patchName = curPatch->file.getFileNameWithoutExtension();

            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::WarningIcon, "Delete Patch",
                "Are you sure you want to delete the following patch:\n\n" + patchName +
                    "\n\nThis cannot be undone!",
                "Yes", "No", nullptr,
                juce::ModalCallbackFunction::create(
                    [this, llp, safeThis = juce::Component::SafePointer(this)](int result) mutable {
                        if (result == 1 && safeThis)
                        {
                            auto &curPatch = utils.patchesAsLinearList[llp];
                            const auto success = curPatch->file.deleteFile();

                            if (success)
                            {
                                utils.rescanPatchTree();
                                llp -= 1;
                                utils.loadPatch(utils.patchesAsLinearList[llp]);
                            }
                        }
                    }));
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Cannot Delete Patch",
                "Factory patches, their unsaved modifications and patches dropped from outside "
                "cannot be deleted!");
        }
    }

    if (action == MenuAction::CopyPatch)
    {
        utils.copyPatch();
    }

    if (action == MenuAction::PastePatch)
    {
        utils.pastePatch();
    }

    if (action == MenuAction::SetDefaultPatch)
    {
        utils.setDefaultPatch(processor.getActiveProgram().getName());
    }

    if (action == MenuAction::RefreshBrowser)
    {
        utils.rescanPatchTree();

        const auto pn = processor.getActiveProgram().getName().toStdString();

        processor.resetLastLoadedProgramByName(pn);
    }

    if (action == MenuAction::RevealUserDirectory)
    {
#if !JUCE_IOS
        utils.getDocumentFolder().revealToUser();
#endif
    }

    if (action == MenuAction::ImportObxdBank)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Import OB-Xd Bank", juce::File(), "*.fxb", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                    importObxdBank(result);
            });
    }

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    // Open Melatonin inspector
    if (action == MenuAction::Inspector)
    {
        this->inspector->setVisible(!this->inspector->isVisible());
        this->inspector->toggle(this->inspector->isVisible());
    }
#endif
}

void ObxfAudioProcessorEditor::keyboardFocusMainMenu()
{
    if (auto *mm = getWidget<ImageMenu>("mainMenu"))
    {
        if (isShowing() && mm->isShowing())
        {
            mm->setWantsKeyboardFocus(true);
            juce::Timer::callAfterDelay(50, [w = juce::Component::SafePointer(mm)]() {
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
}

void ObxfAudioProcessorEditor::importObxdBank(const juce::File &fxbFile)
{
    juce::MemoryBlock fileData;
    if (!fxbFile.loadFileAsData(fileData))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "OB-Xd Bank Import",
                                               "Could not read the selected file!");
        return;
    }

    if (!ObxdImporter::isOBXdData(fileData.getData(), fileData.getSize()))
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "OB-Xd Bank Import",
            "The selected file does not appear to be a valid OB-Xd bank!");
        return;
    }

    const juce::String bankName = fxbFile.getFileNameWithoutExtension();
    const juce::File destFolder = utils.getUserPatchFolder();

    const auto result = ObxdImporter::importBankFromFxb(
        fileData.getData(), fileData.getSize(), bankName, destFolder,
        [this](const Program &prog, juce::MemoryBlock &mb) {
            return utils.serializeProgramToMemoryBlock(prog, mb);
        });

    if (result.parseError)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "OB-Xd Bank Import",
            "The file could not be parsed as a valid OB-Xd bank!");
        return;
    }

    utils.rescanPatchTree();

    const auto pn = processor.getActiveProgram().getName().toStdString();

    processor.resetLastLoadedProgramByName(pn);

    juce::String msg;
    if (result.imported == 0)
    {
        msg = "No patches were imported from \"" + bankName + "\".";
    }
    else
    {
        msg = juce::String(result.imported) + " patch" + (result.imported == 1 ? "" : "es") +
              " imported from \"" + bankName + "\".";
    }

    if (result.skipped > 0)
    {
        msg += "\n" + juce::String(result.skipped) + " init patch" +
               (result.skipped == 1 ? " was" : "es were") + " skipped.";
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "OB-Xd Bank Import", msg);
}

// Patch list

juce::PopupMenu ObxfAudioProcessorEditor::createPatchList(juce::PopupMenu &menu) const
{
    auto lsi = processor.lastLoadedProgram;

    auto raddTo = [that = this, lsi](juce::PopupMenu &m, const Utils::PatchTreeNode::ptr_t &node,
                                     auto &&self) -> void {
        for (auto &child : node->children)
        {
            auto checked = lsi >= child->childRange.first && lsi <= child->childRange.second;
            if (child->isFolder)
            {
                if (!child->children.empty())
                {
                    juce::PopupMenu subMenu;
                    self(subMenu, child, self);
                    m.addSubMenu(child->displayName, subMenu, true, juce::Image(), checked);
                }
            }
            else
            {
                m.addItem(child->displayName, true, checked,
                          [ch = std::weak_ptr(child), w = that]() {
                              if (auto sp = ch.lock())
                              {
                                  w->utils.loadPatch(sp);
                              }
                          });
            }
        }
    };

    for (auto i = 0U; i < utils.patchRoot->children.size(); i++)
    {
        auto &ch = utils.patchRoot->children[i];

        if (!ch->children.empty())
        {
            menu.addSectionHeader(Utils::toString(ch->locationType));
            menu.addSeparator();

            raddTo(menu, ch, raddTo);

            if (i < utils.patchRoot->children.size() - 1)
            {
                menu.addColumnBreak();
            }
        }
    }

    using namespace sst::plugininfra::misc_platform;

    menu.addColumnBreak();
    menu.addSectionHeader("Functions");
    menu.addSeparator();

    bool enablePasteOption = utils.isPatchInClipboard();

    menu.addItem(static_cast<int>(MenuAction::InitializePatch), toOSCase("Initialize Patch"), true,
                 false);

    menu.addSeparator();

    menu.addItem(MenuAction::LoadPatch,
#if JUCE_IOS
                 toOSCase("Import Patch..."),
#else
                 toOSCase("Load Patch..."),
#endif
                 true, false);
    menu.addItem(MenuAction::SavePatch, toOSCase("Save Patch..."), true, false);

    menu.addSeparator();

    menu.addItem(MenuAction::ImportObxdBank, toOSCase("Import ") + "OB-Xd" + toOSCase(" Bank..."),
                 true, false);

    menu.addSeparator();

    menu.addItem(MenuAction::DeletePatch, toOSCase("Delete Patch"), true, false);

    menu.addSeparator();

    menu.addItem(MenuAction::CopyPatch, toOSCase("Copy Patch"), true, false);
    menu.addItem(MenuAction::PastePatch, toOSCase("Paste Patch"), enablePasteOption, false);

    menu.addSeparator();

    /*     menu.addItem(MenuAction::SetDefaultPatch, toOSCase("Set Current Patch as Default"), true,
                     false); */

    menu.addItem(MenuAction::RefreshBrowser, toOSCase("Refresh Patch Browser"), true, false);

    return menu;
}

int ObxfAudioProcessorEditor::patchesInCurrentFolder() const
{
    const auto lsp = processor.lastLoadedPatchNode.lock();

    if (lsp)
    {
        auto lspParent = lsp->parent.lock();

        if (lspParent)
        {
            return lspParent->nonFolderChildIndices.size();
        }
    }

    return 0;
}

// Mutator
void ObxfAudioProcessorEditor::showMutatorMenu()
{
    juce::PopupMenu m;

    m.addSectionHeader("Patch Randomizer");
    m.addSeparator();

    auto custom = std::make_unique<MutatorMenu>(
        processor.mutateSections,
        [this](const int index, bool value) { this->processor.mutateSections[index] = value; });

    m.addCustomItem(1, std::move(custom));

    m.showMenuAsync(obxf::defaultPopupMenuOptions(this));
}

void ObxfAudioProcessorEditor::mutateCallback() { processor.mutatePatch(); }

void ObxfAudioProcessorEditor::randomizeCallback() { processor.randomizePatch(); }