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
#include <unordered_map>
#include <string>

#include "gui/ButtonList.h"
#include "gui/Display.h"
#include "gui/ToggleButton.h"

#include "PluginEditor.h"

struct SaveDialog : juce::Component
{
    static constexpr int noCatID{1000};
    ObxfAudioProcessorEditor &editor;
    std::unique_ptr<juce::Drawable> saveDialogSVG;
    bool hasSkinImage{false};
    std::unordered_map<std::string, juce::Rectangle<int>> boundsMap;
    std::unordered_map<std::string, juce::Colour> colorMap;

    SaveDialog(ObxfAudioProcessorEditor &editor) : editor(editor)
    {
        auto getScaleFactor = [&editor]() { return editor.impliedScaleFactor(); };

        saveDialogSVG = editor.imageCache.getEmbeddedVectorDrawable("label-bg-save-patch");

        resetState();

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

    void resetState()
    {
        hasSkinImage = false;

        if (editor.imageCache.hasImageFor("label-bg-save-patch"))
        {
            hasSkinImage = true;
        }

        boundsMap.clear();
        colorMap.clear();
    }

    void doQuickSave()
    {
        getPatchInfo();
        doSave();
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
        auto lookup = [this](const std::string &key, int dx, int dy, int dw, int dh)
            -> juce::Rectangle<int>
        {
            auto it = boundsMap.find(key);
            if (it != boundsMap.end())
                return it->second;
            return {dx, dy, dw, dh};
        };

        auto nameBounds    = lookup("savePatchNameLabel",      22,  29, 200, 31);
        auto authorBounds  = lookup("savePatchAuthorLabel",    22,  90, 200, 31);
        auto projectBounds = lookup("savePatchProjectLabel",   22, 151, 200, 31);
        auto catBounds     = lookup("savePatchCategoryMenu",   25, 212,  90, 31);
        auto licBounds     = lookup("savePatchLicenseLabel",  126, 212,  96, 31);
        auto cancelBounds  = lookup("savePatchCancelButton",  129, 272,  23, 35);
        auto okBounds      = lookup("savePatchOKButton",       92, 272,  23, 35);

        auto toRect = [&bx, sc](const juce::Rectangle<int> &r)
        {
            return juce::Rectangle<int>(r.getX() * sc + bx.getX(),
                                        r.getY() * sc + bx.getY(),
                                        r.getWidth() * sc,
                                        r.getHeight() * sc);
        };
        // clang-format on

        name->setBounds(toRect(nameBounds));
        author->setBounds(toRect(authorBounds));
        project->setBounds(toRect(projectBounds));
        category->setBounds(toRect(catBounds));
        license->setBounds(toRect(licBounds));
        cancel->setBounds(toRect(cancelBounds));
        ok->setBounds(toRect(okBounds));
    }

    void paint(juce::Graphics &g) override
    {
        auto sc = editor.impliedScaleFactor();

        g.fillAll(juce::Colours::black.withAlpha(0.85f));

        auto r = getContentArea();

        if (hasSkinImage)
        {
            if (editor.imageCache.isSVG("label-bg-save-patch"))
            {
                auto &svg = editor.imageCache.getSVGDrawable("label-bg-save-patch", 0);
                auto at = juce::AffineTransform().scaled(sc).translated(r.getX(), r.getY());
                svg->draw(g, 1.0, at);
            }
            else
            {
                auto img = editor.imageCache.getImageFor("label-bg-save-patch", r.getWidth(),
                                                         r.getHeight());
                g.drawImage(img, r.toFloat());
            }
        }
        else if (saveDialogSVG)
        {
            auto at = juce::AffineTransform().scaled(sc).translated(r.getX(), r.getY());
            saveDialogSVG->draw(g, 1.0, at);
        }
        else
        {
            g.setColour(juce::Colour(0xAA, 0xAA, 0xAA));
            g.fillRect(r);
            g.setColour(juce::Colours::white);
            g.drawRect(r, 3);
        }
    }

    juce::Rectangle<int> getContentArea() const
    {
        auto sc = editor.impliedScaleFactor();

        auto it = boundsMap.find("savePatchDialog");
        int dw = 246, dh = 328;
        if (it != boundsMap.end())
        {
            dw = it->second.getWidth();
            dh = it->second.getHeight();
        }
        else if (saveDialogSVG)
        {
            dw = saveDialogSVG->getWidth();
            dh = saveDialogSVG->getHeight();
        }

        return juce::Rectangle<int>(0, 0, dw * sc, dh * sc)
            .withCentre(getLocalBounds().getCentre());
    }

    void getPatchInfo()
    {
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
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());

        getPatchInfo();

        auto getColor = [this](const std::string &key) -> juce::Colour {
            auto it = colorMap.find(key);
            return it != colorMap.end() ? it->second : juce::Colours::red;
        };

        auto format = [this](Display &comp, juce::Colour color) {
            comp.setFont(editor.patchNameFont.withHeight(18.f));
            comp.setJustificationType(juce::Justification::centred);
            comp.setMinimumHorizontalScale(1.f);
            comp.setColour(juce::Label::textColourId, color);
            comp.setColour(juce::Label::textWhenEditingColourId, color);
            comp.setColour(juce::Label::outlineWhenEditingColourId,
                           juce::Colours::transparentBlack);
            comp.setColour(juce::TextEditor::textColourId, color);
            comp.setColour(juce::TextEditor::highlightedTextColourId, color);
            comp.setColour(juce::TextEditor::highlightColourId, juce::Colour(0x20FFFFFF));
            comp.setColour(juce::CaretComponent::caretColourId, color);
        };

        format(*name, getColor("savePatchNameLabel"));
        format(*project, getColor("savePatchProjectLabel"));
        format(*author, getColor("savePatchAuthorLabel"));
        format(*license, getColor("savePatchLicenseLabel"));

        setVisible(true);
        toFront(true);
    }

    std::unique_ptr<ToggleButton> ok, cancel;
    std::unique_ptr<Display> name, author, license, project;
    std::unique_ptr<juce::ComboBox> category;
};

#endif // OB_XF_SAVEDIALOG_H
