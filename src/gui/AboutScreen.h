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

#ifndef OBXF_SRC_GUI_ABOUTSCREEN_H
#define OBXF_SRC_GUI_ABOUTSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/plugininfra/version_information.h>
#include <sst/plugininfra/paths.h>
#include <sst/plugininfra/cpufeatures.h>

#include "PluginEditor.h"

struct AboutScreen final : juce::Component
{
    ObxfAudioProcessorEditor &editor;
    AboutScreen(ObxfAudioProcessorEditor &editor) : editor(editor) {}

    bool isMouseDown{false};
    void mouseDown(const juce::MouseEvent &) override { isMouseDown = true; }
    void mouseUp(const juce::MouseEvent &event) override
    {
        isMouseDown = false;
        if (buttonRect[0].contains(event.getPosition().toInt()))
        {
            juce::SystemClipboard::copyTextToClipboard(clipboardMsg);
        }
        else if (buttonRect[1].contains(event.getPosition().toInt()))
        {
            juce::URL("https://github.com/surge-synthesizer/OB-Xf").launchInDefaultBrowser();
        }
        else
        {
            setVisible(false);
        }
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.4f));

        auto sfac = editor.impliedScaleFactor();

        auto bxBnd = getLocalBounds().reduced(70 * sfac).withTrimmedBottom(80 * sfac);
        g.setColour(juce::Colours::black);
        g.fillRect(bxBnd);
        g.setColour(juce::Colours::white);
        g.drawRect(bxBnd);

        juce::Graphics::ScopedSaveState ss(g);
        bxBnd = bxBnd.transformedBy(juce::AffineTransform().scaled(sfac).inverted());
        g.addTransform(juce::AffineTransform().scaled(sfac));

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        auto txRec = bxBnd.reduced(8, 4);
        g.setFont(juce::FontOptions(40));
        g.drawText("OB-Xf", txRec, juce::Justification::centredTop);
        txRec = txRec.withTrimmedTop(53);

        g.setFont(juce::FontOptions(20));

        std::vector<std::string> msg = {
            "OB-Xf is a continuation of the last open source version of OB-Xd.",
        };
        g.setColour(juce::Colour(0xE0, 0xE0, 0xE0));
        for (const auto &m : msg)
        {
            g.drawText(m, txRec, juce::Justification::topLeft);
            txRec = txRec.withTrimmedTop(24);
        }

        txRec = txRec.withTrimmedTop(48);

        g.drawText("Click anywhere to close", txRec, juce::Justification::centredTop);

        // txRec = txRec.withTrimmedTop(24);

        clipboardMsg.clear();

        auto drawTag = [&](const auto &a, const auto &b) {
            g.setFont(juce::FontOptions(15));
            g.setColour(juce::Colour(0xFFFF9000));
            g.drawText(a, txRec, juce::Justification::centredLeft);
            g.setColour(juce::Colour(0xFFE0E0E0));
            g.drawText(b, txRec.withTrimmedLeft(100), juce::Justification::centredLeft);

            txRec = txRec.withTrimmedTop(32);
            clipboardMsg += std::string() + a + " : " + b + "\n";
        };

        drawTag("Version:",
                fmt::format("{} | git commit: {}",
                            sst::plugininfra::VersionInformation::git_implied_display_version,
                            sst::plugininfra::VersionInformation::git_commit_hash));
        drawTag("Factory Data:", editor.utils.getFactoryFolder().getFullPathName().toStdString());
        drawTag("User Data:", editor.utils.getDocumentFolder().getFullPathName().toStdString());

        std::string os = "Windows";
#if JUCE_MAC
        os = "macOS";
#else
#if JUCE_LINUX
        os = "Linux";
#endif
#endif
        std::string nm = "unknown";
        auto hs = std::string(juce::PluginHostType().getHostDescription());

        switch (editor.processor.wrapperType)
        {
        case juce::AudioProcessor::wrapperType_Standalone:
            nm = "Standalone";
            break;
        case juce::AudioProcessor::wrapperType_VST3:
            nm = "VST3";
            break;
        case juce::AudioProcessor::wrapperType_AudioUnit:
            nm = "AU";
            break;
        case juce::AudioProcessor::wrapperType_LV2:
            nm = "LV2";
            break;
        default:
            nm = "CLAP";
            break;
        };

        const auto ramsize = juce::SystemStats::getMemorySizeInMegabytes();
        const auto ramString =
            fmt::format("{:.0f} {} RAM", ramsize >= 1024 ? std::roundf(ramsize / 1024.f) : ramsize,
                        ramsize >= 1024 ? "GB" : "MB");

        drawTag("System Info:", fmt::format("{} {} on {}, {}", os, nm,
                                            sst::plugininfra::cpufeatures::brand(), ramString));

        if (editor.processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        {
            drawTag("Sample Rate:", fmt::format("{:d} Hz", (int)editor.processor.getSampleRate()));
        }
        else
        {
            drawTag("Host:", fmt::format("{} @ {} Hz", hs, (int)editor.processor.getSampleRate()));
        }

        auto cpb = txRec.withHeight(28).withWidth(200).translated(0, 30);

        auto fillCol = juce::Colour(20, 20, 20);
        if (cpb.contains(mpos.toInt()))
        {
            if (isMouseDown)
                fillCol = juce::Colour(25, 15, 15);
            else
                fillCol = juce::Colour(45, 40, 40);
        }

        g.setColour(fillCol);
        g.fillRoundedRectangle(cpb.toFloat(), 3);
        g.setColour(juce::Colour(0xFFA0A0A0));
        g.drawRoundedRectangle(cpb.toFloat(), 3, 1);
        g.setColour(juce::Colour(0xFFE0E0E0));
        g.drawText("Copy Info to Clipboard", cpb, juce::Justification::centred);

        // it's a bit crummy to adjust state in paint but quick n dirty here
        buttonRect[0] = cpb;

        // This is really a hack. Clean up one day
        cpb = cpb.translated(cpb.getWidth() + 10, 0);
        fillCol = juce::Colour(20, 20, 20);
        if (cpb.contains(mpos.toInt()))
        {
            if (isMouseDown)
                fillCol = juce::Colour(25, 15, 15);
            else
                fillCol = juce::Colour(45, 40, 40);
        }
        g.setColour(fillCol);
        g.fillRoundedRectangle(cpb.toFloat(), 3);
        g.setColour(juce::Colour(0xFFA0A0A0));
        g.drawRoundedRectangle(cpb.toFloat(), 3, 1);
        g.setColour(juce::Colour(0xFFE0E0E0));
        g.drawText("View on Github", cpb, juce::Justification::centred);
        buttonRect[1] = cpb;
    }

    juce::Rectangle<int> buttonRect[2];
    std::string clipboardMsg;

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        setVisible(true);
        toFront(true);
    }

    juce::Point<float> mpos;
    void mouseEnter(const juce::MouseEvent &event) override
    {
        mpos = event.position;
        repaint();
    }
    void mouseMove(const juce::MouseEvent &event) override
    {
        mpos = event.position;
        repaint();
    }
};

#endif // OBXF_SRC_GUI_ABOUTSCREEN_H
