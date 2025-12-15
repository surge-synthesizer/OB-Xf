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

#ifndef OB_XF_SRC_GUI_SAVEDIALOG_H
#define OB_XF_SRC_GUI_SAVEDIALOG_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/ButtonList.h"
#include "gui/Display.h"
#include "gui/ToggleButton.h"

#include "PluginEditor.h"

struct SaveDialog : juce::Component
{
    static constexpr int noCatID{1000};
    ObxfAudioProcessorEditor &editor;
    std::unique_ptr<juce::Drawable> saveDialogSVG;

    SaveDialog(ObxfAudioProcessorEditor &editor) : editor(editor)
    {
        auto getScaleFactor = [&editor]() { return editor.impliedScaleFactor(); };

        saveDialogSVG = editor.imageCache.getEmbeddedVectorDrawable("label-bg-save-patch");

        ok = std::make_unique<ToggleButton>("button-clear-red", 35, editor.imageCache,
                                            &editor.processor, true);
        ok->onClick = [this] { doSave(); };
        addAndMakeVisible(*ok);

        cancel = std::make_unique<ToggleButton>("button-clear-white", 35, editor.imageCache,
                                                &editor.processor, true);
        cancel->onClick = [this] { setVisible(false); };
        addAndMakeVisible(*cancel);

        name = std::make_unique<Display>("Patch Name", getScaleFactor);
        addAndMakeVisible(*name);

        author = std::make_unique<Display>("Author", getScaleFactor);
        addAndMakeVisible(*author);

        category = std::make_unique<ButtonList>("menu-categories", 31, editor.imageCache,
                                                &editor.processor, true);
        addAndMakeVisible(*category);

        category->addItem("None", noCatID);

        int idx{1};

        for (auto &c : Program::availableCategories())
        {
            category->addItem(c, idx++);
        }

        license = std::make_unique<Display>("License", getScaleFactor);
        addAndMakeVisible(*license);

        project = std::make_unique<Display>("Project", getScaleFactor);
        addAndMakeVisible(*project);
    }

    void doSave()
    {
        OBLOG(patchSave, "Starting patch save");

        auto pth = editor.utils.getUserPatchFolder();

        if (project->getText().isNotEmpty())
        {
            pth = pth.getChildFile(project->getText());
        }
        else if (category->getSelectedId() != noCatID)
        {
            pth = pth.getChildFile(category->getText());
        }

        if (name->getText().toStdString() == INIT_PATCH_NAME)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Reserved Patch Name",
                fmt::format("\"{}\" is a reserved patch name for internal use. Please choose "
                            "another name for your patch!",
                            INIT_PATCH_NAME));
            return;
        }

        pth = pth.getChildFile(name->getText() + ".fxp");

        OBLOG(patchSave, "Saving patch to " << pth.getFullPathName());

        auto &pr = editor.processor.getActiveProgram();
        auto pj = project->getText();

        for (auto &c : Program::availableCategories())
        {
            if (c.compareIgnoreCase(pj) == 0)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Invalid Project Name",
                    "Project name cannot be any of the available patch category names. Please "
                    "choose another name for your project!");
                return;
            }
        }

        pr.setName(name->getText());
        pr.setAuthor(author->getText());
        editor.utils.setLastPatchAuthor(author->getText());

        pr.setLicense(license->getText());
        editor.utils.setLastPatchLicense(license->getText());

        pr.setCategory(category->getSelectedId() == noCatID ? "" : category->getText());
        pr.setProject(project->getText());

        if (!editor.utils.savePatch(pth))
        {
            OBLOG(patchSave, "Failed to save patch");
        }

        editor.processor.resetLastLoadedProgramByName(pr.getName().toStdString(), true);

        setVisible(false);
    }

    void resized() override
    {
        auto sc = editor.impliedScaleFactor();
        auto bx = getContentArea();

        // clang-format off
        float nameBounds   [4] {  22,  29, 200, 31 };
        float authorBounds [4] {  22,  90, 200, 31 };
        float projectBounds[4] {  22, 151, 200, 31 };
        float catBounds    [4] {  25, 212,  90, 31 };
        float licBounds    [4] { 126, 212,  96, 31 };
        float cancelBounds [4] { 129, 272,  23, 35 };
        float okBounds     [4] {  92, 272,  23, 35 };

        auto toR = [&bx, sc](auto *v)
        {
            return juce::Rectangle<int>(v[0] * sc + bx.getX(),
                                        v[1] * sc + bx.getY(),
                                        v[2] * sc,
                                        v[3] * sc);
        };
        // clang-format on

        name->setBounds(toR(nameBounds));
        author->setBounds(toR(authorBounds));
        project->setBounds(toR(projectBounds));
        category->setBounds(toR(catBounds));
        license->setBounds(toR(licBounds));
        cancel->setBounds(toR(cancelBounds));
        ok->setBounds(toR(okBounds));
    }

    void paint(juce::Graphics &g) override
    {
        auto sc = editor.impliedScaleFactor();

        g.fillAll(juce::Colours::black.withAlpha(0.85f));

        if (saveDialogSVG)
        {
            auto r = getContentArea();
            auto at = juce::AffineTransform().scaled(sc).translated(r.getX(), r.getY());
            saveDialogSVG->draw(g, 1.0, at);
        }
        else
        {
            g.setColour(juce::Colour(0xAA, 0xAA, 0xAA));
            g.fillRect(getContentArea());
            g.setColour(juce::Colours::white);
            g.drawRect(getContentArea(), 3);
        }
    }

    juce::Rectangle<int> getContentArea() const
    {
        auto sc = editor.impliedScaleFactor();

        if (!saveDialogSVG)
        {
            auto bx = juce::Rectangle<int>(0, 0, 246 * sc, 328 * sc)
                          .withCentre(getLocalBounds().getCentre());
            return bx;
        }
        else
        {
            auto w = saveDialogSVG->getWidth();
            auto h = saveDialogSVG->getHeight();

            return juce::Rectangle<int>(0, 0, w * sc, h * sc)
                .withCentre(getLocalBounds().getCentre());
        }
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());

        auto &pr = editor.processor.getActiveProgram();

        name->setText(pr.getName(), juce::dontSendNotification);

        if (pr.getAuthor().isNotEmpty())
            author->setText(pr.getAuthor(), juce::dontSendNotification);
        else
            author->setText(editor.utils.getLastPatchAuthor(), juce::dontSendNotification);

        if (pr.getLicense().isNotEmpty())
            license->setText(pr.getLicense(), juce::dontSendNotification);
        else
            license->setText(editor.utils.getLastPatchLicense(), juce::dontSendNotification);

        project->setText(pr.getProject(), juce::dontSendNotification);

        category->setSelectedId(noCatID, juce::dontSendNotification);

        auto format = [this](Display &comp) {
            comp.setFont(editor.patchNameFont.withHeight(18.f));
            comp.setJustificationType(juce::Justification::centred);
            comp.setMinimumHorizontalScale(1.f);
            comp.setColour(juce::Label::textColourId, juce::Colours::red);
            comp.setColour(juce::Label::textWhenEditingColourId, juce::Colours::red);
            comp.setColour(juce::Label::outlineWhenEditingColourId,
                           juce::Colours::transparentBlack);
            comp.setColour(juce::TextEditor::textColourId, juce::Colours::red);
            comp.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::red);
            comp.setColour(juce::TextEditor::highlightColourId, juce::Colour(0x30FFFFFF));
            comp.setColour(juce::CaretComponent::caretColourId, juce::Colours::red);
        };

        format(*name);
        format(*project);
        format(*author);
        format(*license);

        int idx{1};

        for (auto &c : Program::availableCategories())
        {
            if (pr.getCategory() == c)
            {
                category->setSelectedId(idx, juce::dontSendNotification);
                break;
            }

            idx++;
        }

        setVisible(true);

        toFront(true);
    }

    std::unique_ptr<ToggleButton> ok, cancel;
    std::unique_ptr<Display> name, author, license, project;
    std::unique_ptr<juce::ComboBox> category;
};

#endif // OB_XF_SAVEDIALOG_H
