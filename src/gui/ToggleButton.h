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

#ifndef OBXF_SRC_GUI_TOGGLEBUTTON_H
#define OBXF_SRC_GUI_TOGGLEBUTTON_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalingImageCache.h"
#include "HasScaleFactor.h"

class ToggleButton final : public juce::ImageButton, public HasScaleFactor
{
    juce::String img_name;
    ScalingImageCache &imageCache;
    bool isSVG{false};

  public:
    ToggleButton(juce::String name, const int fh, ScalingImageCache &cache,
                 ObxfAudioProcessor *owner_)
        : img_name(std::move(name)), imageCache(cache), owner(owner_)
    {
        scaleFactorChanged();

        if (!isSVG)
        {
            width = kni.getWidth();
            height = kni.getHeight();
        }
        else
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            width = svgi->getWidth();
            height = svgi->getHeight();
        }
        h2 = fh;
        w2 = width;
        if (h2 > 0)
            numFr = height / h2;
        else
            numFr = 1;

        setClickingTogglesState(true);
    }

    void scaleFactorChanged() override
    {
        if (imageCache.isSVG(img_name.toStdString()))
        {
            isSVG = true;
        }
        else
        {
            isSVG = false;
            kni = imageCache.getImageFor(img_name.toStdString(), getWidth(), h2);
        }

        repaint();
    }

    void resized() override { scaleFactorChanged(); }

    ~ToggleButton() override = default;

    void paintButton(juce::Graphics &g, bool /*isMouseOverButton*/, bool isButtonDown) override
    {
        int offset = isButtonDown ? 1 : 0;
        if (getToggleState() && numFr > 2)
            offset += 2;

        if (isSVG)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(0, -scale * h2 * offset);
            svgi->draw(g, 1.f, tf);
        }
        else
        {
            const int zoomLevel =
                imageCache.zoomLevelFor(img_name.toStdString(), getWidth(), getHeight());
            constexpr int baseZoomLevel = 100;
            const float scale = static_cast<float>(zoomLevel) / static_cast<float>(baseZoomLevel);

            const int srcW = w2 * scale;
            const int srcH = h2 * scale;
            const int srcY = offset * h2 * scale;

            g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, srcY, srcW, srcH);
        }
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.mods.isRightButtonDown())
        {
            showPopupMenu();
        }
        else
        {
            if (owner && parameter)
            {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                {
                    obxf->setLastUsedParameter(parameter->paramID);
                }
            }
        }

        ImageButton::mouseDown(event);
    }

    bool keyPressed(const juce::KeyPress &e) override
    {
        if (e.getModifiers().isShiftDown() && e.getKeyCode() == juce::KeyPress::F10Key)
        {
            showPopupMenu();
            return true;
        }

        return juce::ImageButton::keyPressed(e);
    }

    void setParameter(juce::AudioProcessorParameterWithID *p) { parameter = p; }

    void showPopupMenu()
    {
        using namespace sst::plugininfra::misc_platform;

        juce::PopupMenu menu;

        if (getTitle().compare(SynthParam::Name::SavePatch) == 0)
        {
            auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner);

            if (obxf)
            {
                menu.addSectionHeader("Save Options");

                menu.addSeparator();

                menu.addItem(toOSCase("Save All Patches in Bank"),
                             [obxf]() { obxf->saveAllFrontProgramsToBack(); });

                menu.addItem(toOSCase("Save All Patches in Current Group"), [obxf]() {
                    const uint8_t curGrp = obxf->getCurrentProgram() / NUM_PATCHES_PER_GROUP;

                    for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
                    {
                        obxf->saveSpecificFrontProgramToBack((curGrp * NUM_PATCHES_PER_GROUP) + i);
                    }
                });
            }
        }

        if (auto name = getTitle(); std::isdigit(name.getLastCharacter()))
        {
            auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner);

            if (obxf)
            {
                auto which = name.retainCharacters("0123456789").getIntValue();
                const int idx = (obxf->getCurrentPatchGroup() * NUM_PATCHES_PER_GROUP) + which;

                menu.addSectionHeader("Patch Options");

                menu.addSeparator();

                menu.addItem(toOSCase(std::format("Copy Patch {}", idx)),
                             [obxf, idx]() { obxf->utils->copyPatch(idx - 1); });

                bool ticked = obxf->utils->isPatchInClipboard();

                menu.addItem(toOSCase(std::format("Paste to Patch {}", idx)), ticked, false,
                             [obxf, idx]() { obxf->utils->pastePatch(obxf, idx - 1); });
            }
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(getTopLevelComponent()));
    }

  private:
    juce::Image kni;
    int width, height, numFr, w2, h2;
    juce::AudioProcessor *owner{nullptr};
    juce::AudioProcessorParameterWithID *parameter{nullptr};
};

#endif // OBXF_SRC_GUI_TOGGLEBUTTON_H
