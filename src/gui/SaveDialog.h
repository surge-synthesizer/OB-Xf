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
#include "PluginEditor.h"
#include "gui/ToggleButton.h"
#include "gui/ButtonList.h"

struct SaveDialog : juce::Component
{
    static constexpr int noCatID{1000};
    ObxfAudioProcessorEditor &editor;
    std::unique_ptr<juce::Drawable> saveDialogSVG;
    SaveDialog(ObxfAudioProcessorEditor &editor) : editor(editor)
    {
        saveDialogSVG = editor.imageCache.getEmbeddedVectorDrawable("label-bg-save-patch");
        saveB = std::make_unique<ToggleButton>("button-clear-red", 35, editor.imageCache,
                                               &editor.processor, true);
        saveB->onClick = [this] { doSave(); };
        addAndMakeVisible(*saveB);

        cancelB = std::make_unique<ToggleButton>("button-clear-white", 35, editor.imageCache,
                                                 &editor.processor, true);
        cancelB->onClick = [this] { setVisible(false); };
        addAndMakeVisible(*cancelB);

        name = std::make_unique<juce::TextEditor>();
        name->setMultiLine(false);
        addAndMakeVisible(*name);

        author = std::make_unique<juce::TextEditor>();
        author->setMultiLine(false);
        addAndMakeVisible(*author);

        category = std::make_unique<ButtonList>("menu-categories", 31, editor.imageCache,
                                                &editor.processor, true);
        addAndMakeVisible(*category);

        category->addItem("Empty", noCatID);

        int idx{1};

        for (auto &c : Program::availableCategories())
        {
            category->addItem(c, idx++);
        }

        license = std::make_unique<juce::TextEditor>();
        license->setMultiLine(false);
        addAndMakeVisible(*license);

        project = std::make_unique<juce::TextEditor>();
        project->setMultiLine(false);
        addAndMakeVisible(*project);
    }

    void doSave()
    {
        OBLOG(patchSave, "Starting patch save");
        auto pth = editor.utils.getUserPatchFolder();
        if (project->getText().isNotEmpty())
            pth = pth.getChildFile(project->getText());
        else if (category->getSelectedId() != noCatID)
            pth = pth.getChildFile(category->getText());

        if (name->getText().toStdString() == INIT_PATCH_NAME)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Cant Save with Init Name",
                "We reserve the INIT_PATCH_NAME for the factory set. Sorry!!");
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
                    juce::AlertWindow::WarningIcon, "Project Name cant be Category",
                    "Project Name collides with known categories. Sorry!");
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

        setVisible(false);
    }

    void resized() override
    {
        auto sc = editor.impliedScaleFactor();
        auto bx = getContentArea();

        float cancelP[4]{92, 272, 23, 35};
        float saveP[4]{129, 272, 23, 35};

        float nameP[4]{25.5, 29.5, 193, 30};
        float authorP[4]{25.5, 90.5, 193, 30};
        float projectP[4]{25.5, 151.5, 193, 30};
        float liceP[4]{129.5, 212.5, 89, 30};
        float catP[4]{25.5, 212.5, 89, 30};

        auto toR = [&bx, sc](auto *v) {
            return juce::Rectangle<int>(1.f * bx.getX() + v[0] * sc, 1.f * bx.getY() + v[1] * sc,
                                        v[2] * sc, v[3] * sc);
        };
        cancelB->setBounds(toR(cancelP));
        saveB->setBounds(toR(saveP));

        name->setBounds(toR(nameP));
        author->setBounds(toR(authorP));
        project->setBounds(toR(projectP));
        category->setBounds(toR(catP));
        license->setBounds(toR(liceP));
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
            auto bx = juce::Rectangle<int>(0, 0, 500 * sc, 280 * sc)
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

        auto sc = editor.impliedScaleFactor();
        auto format = [this, sc](juce::TextEditor &comp) {
            auto font(editor.patchNameFont.withHeight(18.f * sc));
            comp.setFont(font);
            comp.setJustification(juce::Justification::centredLeft);
            comp.setIndents(3, 3);
            comp.setColour(juce::TextEditor::textColourId, juce::Colours::red);
            comp.setColour(juce::TextEditor::ColourIds::outlineColourId,
                           juce::Colours::black.withAlpha(0.f));
            comp.setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
                           juce::Colours::black.withAlpha(0.f));
            comp.setColour(juce::TextEditor::ColourIds::backgroundColourId,
                           juce::Colours::black.withAlpha(0.f));
            comp.applyFontToAllText(comp.getFont());
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

    std::unique_ptr<ToggleButton> saveB, cancelB;
    std::unique_ptr<juce::TextEditor> name, author, license, project;
    std::unique_ptr<juce::ComboBox> category;
};

#endif // OB_XF_SAVEDIALOG_H
