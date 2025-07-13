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

#include "PluginEditor.h"

struct AboutScreen final : juce::Component
{
    ObxfAudioProcessorEditor &editor;
    AboutScreen(ObxfAudioProcessorEditor &editor) : editor(editor) {}

    void mouseUp(const juce::MouseEvent & /*event*/) override { setVisible(false); }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.4f));

        const auto bxBnd = getLocalBounds().reduced(70).withTrimmedBottom(80);
        g.setColour(juce::Colours::black);
        g.fillRect(bxBnd);
        g.setColour(juce::Colours::white);
        g.drawRect(bxBnd);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        auto txRec = bxBnd.reduced(8, 4);
        g.setFont(juce::FontOptions(40));
        g.drawText("OB-Xf", txRec, juce::Justification::centredTop);
        txRec = txRec.withTrimmedTop(53);

        g.setFont(juce::FontOptions(20));

        std::vector<std::string> msg = {
            "OB-Xf is A continuation of the last open source version of OB-Xd.",
        };
        g.setColour(juce::Colour(0xE0, 0xE0, 0xE0));
        for (const auto &m : msg)
        {
            g.drawText(m, txRec, juce::Justification::topLeft);
            txRec = txRec.withTrimmedTop(24);
        }

        txRec = txRec.withTrimmedTop(50);
        g.drawText("Click anywhere to close", txRec, juce::Justification::centredTop);

        txRec = txRec.withTrimmedTop(130);

        auto drawTag = [&](const auto &a, const auto &b) {
            g.setFont(juce::FontOptions(15));
            g.setColour(juce::Colour(0xFF, 0x90, 0x00));
            g.drawText(a, txRec, juce::Justification::topLeft);
            g.setColour(juce::Colour(0xE0, 0xE0, 0xE0));
            g.drawText(b, txRec.withTrimmedLeft(100), juce::Justification::topLeft);
            txRec = txRec.withTrimmedTop(20);
        };
        drawTag("Version", std::string() +
                               sst::plugininfra::VersionInformation::git_implied_display_version +
                               " / " + sst::plugininfra::VersionInformation::git_commit_hash);
        drawTag("User Dir", editor.utils.getDocumentFolder().getFullPathName().toStdString());
        std::string os = "windows";
#if JUCE_MAC
        os = "macOS";
#else
#if JUCE_LINUX
        os = "linux";
#endif
#endif
        drawTag("OS", os + " " + sst::plugininfra::VersionInformation::cmake_compiler);

        std::string nm = "unk";
        auto hs = std::string(juce::PluginHostType().getHostDescription());
        switch (editor.processor.wrapperType)
        {
        case juce::AudioProcessor::wrapperType_Standalone:
            nm = "Standalone";
            break;
        case juce::AudioProcessor::wrapperType_VST3:
            nm = "VST3 in " + hs;
            break;
        case juce::AudioProcessor::wrapperType_AudioUnit:
            nm = "AudioUnit in " + hs;
            break;
        case juce::AudioProcessor::wrapperType_LV2:
            nm = "LV2 in " + hs;
            break;
        default:
            nm = "CLAP in " + hs;
            break;
        };
        drawTag("Environment", nm + " @ " + std::to_string((int)editor.processor.getSampleRate()));
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        setVisible(true);
        toFront(true);
    }
};

#endif // OBXF_SRC_GUI_ABOUTSCREEN_H
