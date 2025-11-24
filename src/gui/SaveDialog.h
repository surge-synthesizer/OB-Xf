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

struct SaveDialog : juce::Component
{
    static constexpr int noCatID{1000};
    ObxfAudioProcessorEditor &editor;
    SaveDialog(ObxfAudioProcessorEditor &editor) : editor(editor)
    {
        saveB = std::make_unique<juce::TextButton>("Save");
        saveB->onClick = [this] { doSave(); };
        addAndMakeVisible(*saveB);
        cancelB = std::make_unique<juce::TextButton>("Cancel");
        cancelB->onClick = [this] { setVisible(false); };
        addAndMakeVisible(*cancelB);

        name = std::make_unique<juce::TextEditor>();
        name->setMultiLine(false);
        addAndMakeVisible(*name);

        author = std::make_unique<juce::TextEditor>();
        author->setMultiLine(false);
        addAndMakeVisible(*author);

        category = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(*category);
        int idx{1};
        for (auto &c : Program::availableCategories())
        {
            category->addItem(c, idx++);
        }
        category->addItem("None", noCatID);

        license = std::make_unique<juce::TextEditor>();
        license->setMultiLine(false);
        addAndMakeVisible(*license);

        project = std::make_unique<juce::TextEditor>();
        project->setMultiLine(false);
        addAndMakeVisible(*project);

        includeCatInPath = std::make_unique<juce::ToggleButton>();
        includeCatInPath->setToggleState(editor.utils.getCategoryPathSaveOption(),
                                         juce::dontSendNotification);
        addAndMakeVisible(*includeCatInPath);
    }

    void doSave()
    {
        OBLOG(patchSave, "Starting patch save");
        auto pth = editor.utils.getUserPatchFolder();
        if (project->getText().isNotEmpty())
            pth = pth.getChildFile(project->getText());
        if (includeCatInPath->getToggleState() && category->getSelectedId() != noCatID)
            pth = pth.getChildFile(category->getText());

        pth = pth.getChildFile(name->getText() + ".fxp");

        OBLOG(patchSave, "Saving patch to " << pth.getFullPathName());

        auto &pr = editor.processor.getActiveProgram();
        pr.setName(name->getText());
        pr.setAuthor(author->getText());
        pr.setLicense(license->getText());
        pr.setCategory(category->getSelectedId() == noCatID ? "" : category->getText());

        if (!editor.utils.savePatch(pth))
        {
            OBLOG(patchSave, "Failed to save patch");
        }

        editor.utils.setCategoryPathSaveOption(includeCatInPath->getToggleState());
        setVisible(false);
    }

    void resized() override
    {
        auto sc = editor.impliedScaleFactor();
        auto bx = getContentArea();

        auto bs = bx.removeFromBottom(40 * sc);
        auto ca = bs.removeFromRight(80 * sc);
        cancelB->setBounds(ca.reduced(3 * sc));
        ca = ca.translated(-80 * sc, 0);
        saveB->setBounds(ca.reduced(3 * sc));

        auto tia = bx.removeFromTop(40 * sc).withTrimmedLeft(100 * sc);
        name->setBounds(tia.reduced(3 * sc));
        tia = tia.translated(0, 40 * sc);
        author->setBounds(tia.reduced(3 * sc));
        tia = tia.translated(0, 40 * sc);
        license->setBounds(tia.reduced(3 * sc));
        tia = tia.translated(0, 40 * sc);
        project->setBounds(tia.reduced(3 * sc));
        tia = tia.translated(0, 40 * sc);
        category->setBounds(tia.reduced(3 * sc));
        tia = tia.translated(0, 40 * sc);
        includeCatInPath->setBounds(tia.reduced(3 * sc).withWidth(34 * sc));
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.85f));
        auto bx = getContentArea();
        g.setColour(juce::Colour(20, 20, 60));
        g.fillRect(bx);
        g.setColour(juce::Colours::white);
        g.drawRect(bx);

        auto sc = editor.impliedScaleFactor();
        auto tia = bx.removeFromTop(40 * sc);
        g.setFont(juce::FontOptions(14 * sc));
        g.drawText("Name:", tia.reduced(3 * sc), juce::Justification::centredLeft);
        tia = tia.translated(0, 40 * sc);
        g.drawText("Author:", tia.reduced(3 * sc), juce::Justification::centredLeft);
        tia = tia.translated(0, 40 * sc);
        g.drawText("License:", tia.reduced(3 * sc), juce::Justification::centredLeft);
        tia = tia.translated(0, 40 * sc);
        g.drawText("Project:", tia.reduced(3 * sc), juce::Justification::centredLeft);
        tia = tia.translated(0, 40 * sc);
        g.drawText("Category:", tia.reduced(3 * sc), juce::Justification::centredLeft);
        tia = tia.translated(0, 40 * sc);
        g.drawText("Cat in Path:", tia.reduced(3 * sc), juce::Justification::centredLeft);
    }

    juce::Rectangle<int> getContentArea() const
    {
        auto sc = editor.impliedScaleFactor();
        auto bx =
            juce::Rectangle<int>(0, 0, 500 * sc, 280 * sc).withCentre(getLocalBounds().getCentre());
        return bx;
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        auto &pr = editor.processor.getActiveProgram();
        name->setText(pr.getName(), juce::dontSendNotification);
        if (pr.getAuthor().isNotEmpty())
            author->setText(pr.getAuthor(), juce::dontSendNotification);
        else
            author->setText("Author Unknown", juce::dontSendNotification);
        if (pr.getLicense().isNotEmpty())
            license->setText(pr.getLicense(), juce::dontSendNotification);
        else
            license->setText("CC0/Public Domain", juce::dontSendNotification);

        category->setSelectedId(noCatID, juce::dontSendNotification);
        int idx{1};
        for (auto &c : Program::availableCategories())
        {
            if (pr.getCategory() == c)
                category->setSelectedId(idx, juce::dontSendNotification);
        }
        setVisible(true);

        auto llp = editor.processor.lastLoadedPatchNode.lock();
        if (llp)
        {
            auto lf = llp->file;
            OBLOG(patchSave, "Last loaded patch: " << lf.getFullPathName());
            if (lf.isAChildOf(editor.utils.getUserPatchFolder()))
            {
                auto p = lf.getRelativePathFrom(editor.utils.getUserPatchFolder());
                auto lastP = p.lastIndexOfAnyOf("/\\");
                p = p.substring(0, lastP);
                if (p.isNotEmpty())
                    project->setText(p, juce::dontSendNotification);
                OBLOG(patchSave, p);
            }
        }

        toFront(true);
    }

    std::unique_ptr<juce::TextButton> saveB, cancelB;
    std::unique_ptr<juce::TextEditor> name, author, license, project;
    std::unique_ptr<juce::ComboBox> category;
    std::unique_ptr<juce::ToggleButton> includeCatInPath;
};

#endif // OB_XF_SAVEDIALOG_H
