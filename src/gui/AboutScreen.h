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
#include "BinaryData.h"

struct AboutScreen final : juce::Component
{
    ObxfAudioProcessorEditor &editor;
    AboutScreen(ObxfAudioProcessorEditor &editor) : editor(editor) {}

    bool isMouseDown{false};
    juce::Point<float> mpos;
    std::string clipboardMsg;

    // All rectangles with the name 'scaled' are in pixel coordinates
    // and ones without are in the unzoomed coordinates
    juce::Rectangle<int> scaledAboutBounds, aboutBounds;

    static constexpr int numButtons{9};
    static constexpr int iconSize{36};
    static constexpr int margin{16};

    std::vector<std::string> buttonLabels{"Copy Info to Clipboard"};
    // clang-format off
    std::vector<std::function<void()>> buttonActions
    {
        [this]() { juce::SystemClipboard::copyTextToClipboard(clipboardMsg); },
        []() { juce::URL("https://github.com/surge-synthesizer/OB-Xf").launchInDefaultBrowser(); },
        []() { juce::URL("https://discord.gg/aFQDdMV").launchInDefaultBrowser(); },
        []() { juce::URL("https://www.gnu.org/licenses/gpl-3.0-standalone.html").launchInDefaultBrowser(); },
        []() { juce::URL("https://cleveraudio.org").launchInDefaultBrowser(); },
        []() { juce::URL("https://lv2plug.in").launchInDefaultBrowser(); },
        []() { juce::URL("https://www.steinberg.net/en/company/technologies/vst3.html").launchInDefaultBrowser(); },
        []() { juce::URL("https://developer.apple.com/documentation/audiounit").launchInDefaultBrowser(); },
        []() { juce::URL("https://juce.com").launchInDefaultBrowser(); },
    };
    // clang-format on
    juce::Rectangle<int> scaledButtonRect[numButtons], buttonRect[numButtons];

    juce::String iconsPath =
        juce::String::fromUTF8(BinaryData::iconlinks_svg, BinaryData::iconlinks_svgSize);

    std::unique_ptr<juce::XmlElement> xml1 = juce::XmlDocument::parse(iconsPath);
    std::unique_ptr<juce::Drawable> icons = juce::Drawable::createFromSVG(*xml1);

    juce::String logoPath = juce::String::fromUTF8(BinaryData::logo_svg, BinaryData::logo_svgSize);

    std::unique_ptr<juce::XmlElement> xml2 = juce::XmlDocument::parse(logoPath);
    std::unique_ptr<juce::Drawable> logo = juce::Drawable::createFromSVG(*xml2);

    ////////////////////////////////

    void mouseDown(const juce::MouseEvent &) override { isMouseDown = true; }

    void mouseUp(const juce::MouseEvent &event) override
    {
        isMouseDown = false;

        for (int i = 0; i < numButtons; i++)
        {
            if (scaledButtonRect[i].contains(event.getPosition().toInt()))
            {
                buttonActions[i]();
                return;
            }
        }

        setVisible(false);
    }

    void resized() override
    {
        auto sfac = editor.impliedScaleFactor();

        auto scaledToUnzoomed = juce::AffineTransform().scaled(sfac).inverted();
        auto unzoomedToScaled = juce::AffineTransform().scaled(sfac);

        scaledAboutBounds = getLocalBounds();
        aboutBounds = scaledAboutBounds.transformedBy(scaledToUnzoomed);

        auto ba = aboutBounds.reduced(margin).withTop(aboutBounds.getBottom() - margin * 2);

        int x = 8; // number of iconized links

        buttonRect[0] = ba.translated(0, -margin * 9.5f).withWidth(133);

        for (int i = 1; i < numButtons; i++)
        {
            buttonRect[i] = juce::Rectangle<int>(aboutBounds.getWidth() - (margin / 2) - (x * 42),
                                                 margin, iconSize, iconSize);

            x--;
        }

        for (int i = 0; i < numButtons; i++)
        {
            scaledButtonRect[i] = buttonRect[i].transformedBy(unzoomedToScaled);
        }
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.85f));

        auto sfac = editor.impliedScaleFactor();

        juce::Graphics::ScopedSaveState ss(g);

        g.addTransform(juce::AffineTransform().scaled(sfac));

        // From here onwards we are painting in unzoomed coordinates
        // so you can just use pixel units we see fit

        auto lRec = aboutBounds.withWidth(500).withHeight(165).withCentre(aboutBounds.getCentre());

        logo->drawWithin(g, lRec.toFloat(), juce::RectanglePlacement::stretchToFit, 1.f);

        auto txRec = aboutBounds.reduced(margin);

        g.setFont(juce::FontOptions(14));

        // clang-format off
        std::vector<std::string> msg =
        {
            "OB-Xf is a continuation of the last free and open source version of OB-Xd by Vadim Filatov (2DaT).",
            "Copyright by Vadim Filatov and individual contributors in the Surge Synth Team, released under the GNU GPL v3 license.",
            "CLAP support is licensed under MIT license.",
            "VST is a trademark of Steinberg Media Technologies GmbH.",
            "Audio Units is a trademark of Apple Inc.",
        };
        // clang-format on

        g.setColour(juce::Colours::white);

        for (const auto &m : msg)
        {
            g.drawText(m, txRec, juce::Justification::topLeft);
            txRec = txRec.withTrimmedTop(16);
        }

        clipboardMsg.clear();

        auto infoRec = aboutBounds.reduced(margin).withTop(aboutBounds.getBottom() - margin * 2);

        auto drawTag = [&](const auto &a, const auto &b, const int offset) {
            auto r = infoRec.translated(0, -margin * offset);

            g.setFont(juce::FontOptions(14));
            g.setColour(juce::Colour(0xFFFF9000));
            g.drawText(a, r, juce::Justification::centredLeft);
            g.setColour(juce::Colours::white);
            g.drawText(b, r.withTrimmedLeft(100), juce::Justification::centredLeft);

            clipboardMsg += std::string() + a + " : " + b + "\n";
        };

        using ver = sst::plugininfra::VersionInformation;

        drawTag("Version:", ver::project_version_and_hash, 8);
        drawTag("Build Info:",
                fmt::format("{} @ {} on {} with {} using JUCE {}.{}.{}", ver::build_date,
                            ver::build_time, ver::build_host, ver::cmake_compiler,
                            JUCE_MAJOR_VERSION, JUCE_MINOR_VERSION, JUCE_BUILDNUMBER),
                7);

        std::string os = "Windows";
#if JUCE_MAC
        os = "macOS";
#else
#if JUCE_LINUX
        os = "Linux";
#endif
#endif
        std::string nm = "Unknown";
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

        drawTag("System Info:",
                fmt::format("{} {} on {}, {}", os, nm, sst::plugininfra::cpufeatures::brand(),
                            ramString),
                6);

        if (editor.processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        {
            drawTag("Sample Rate:", fmt::format("{:d} Hz", (int)editor.processor.getSampleRate()),
                    5);
        }
        else
        {
            drawTag("Host:", fmt::format("{} @ {} Hz", hs, (int)editor.processor.getSampleRate()),
                    5);
        }

        drawTag("Executable:", sst::plugininfra::paths::sharedLibraryBinaryPath().string(), 2);
        drawTag("Factory Data:",
                editor.utils.getFactoryFolderInUse().getFullPathName().toStdString(), 1);
        drawTag("User Data:", editor.utils.getDocumentFolder().getFullPathName().toStdString(), 0);

        ////////////////////////////

        for (int i = 0; i < numButtons; i++)
        {
            auto &r = buttonRect[i];
            const bool isOverRect = scaledButtonRect[i].contains(mpos.toInt());

            if (i == 0)
            {
                auto color = isOverRect ? juce::Colour(0xFF60C4FF) : juce::Colour(0xFF2D86FE);

                g.setColour(color);
                g.drawText(buttonLabels[i], r, juce::Justification::centredLeft);
            }
            else
            {
                int ys = isOverRect ? iconSize : 0;
                int xs = (i - 1) * iconSize;

                juce::Graphics::ScopedSaveState gs(g);

                auto t = juce::AffineTransform();
                t = t.translated(r.getX(), r.getY()).translated(-xs, -ys);
                g.reduceClipRegion(r);

                icons->draw(g, 1.f, t);
            }
        }
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        setVisible(true);
        toFront(true);
    }

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
