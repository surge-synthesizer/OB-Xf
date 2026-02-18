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

#ifndef OBXF_SRC_GUI_KNOB_H
#define OBXF_SRC_GUI_KNOB_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalingImageCache.h"
#include "HasScaleFactor.h"

#include "sst/plugininfra/misc_platform.h"

class ObxfAudioProcessor;

class KnobLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    KnobLookAndFeel()
    {
        setColour(juce::BubbleComponent::ColourIds::backgroundColourId,
                  juce::Colour(48, 48, 48).withAlpha(0.8f));
        setColour(juce::BubbleComponent::ColourIds::outlineColourId,
                  juce::Colour(64, 64, 64).withAlpha(0.6f));
        setColour(juce::TooltipWindow::textColourId, juce::Colours::white);
    }

    int getSliderPopupPlacement(juce::Slider &) override
    {
        return juce::BubbleComponent::BubblePlacement::above;
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobLookAndFeel)
};

class Knob final : public juce::Slider,
                   public juce::ActionBroadcaster,
                   public HasScaleFactor,
                   public HasParameterWithID
{
    juce::String img_name;
    ScalingImageCache &imageCache;

    struct MenuValueTypein final : juce::PopupMenu::CustomComponent, juce::TextEditor::Listener
    {
        SafePointer<Knob> knob;
        std::unique_ptr<juce::TextEditor> textEditor;
        juce::AudioProcessor *owner{nullptr};

        explicit MenuValueTypein(juce::AudioProcessor &p, Knob *k)
            : juce::PopupMenu::CustomComponent(false), knob(k), owner(&p)
        {
            textEditor = std::make_unique<juce::TextEditor>();
            textEditor->addListener(this);
            textEditor->setWantsKeyboardFocus(true);
            textEditor->setIndents(2, 0);
            textEditor->setCaretVisible(true);
            textEditor->setReadOnly(false);
            textEditor->setTitle(k->parameter->getName(128));

            addAndMakeVisible(*textEditor);
        }

        void getIdealSize(int &width, int &height) override
        {
            width = 120;
            height = 28;
        }

        void resized() override { textEditor->setBounds(getLocalBounds().reduced(3, 1)); }

        void visibilityChanged() override
        {
            juce::Timer::callAfterDelay(2, [this]() {
                if (textEditor->isVisible() && knob)
                {
                    auto txt = juce::String(knob->getValue());
                    auto md = knob->getMetadata();

                    if (md.has_value())
                    {
                        const float denorm = juce::jmap(static_cast<float>(knob->getValue()), 0.0f,
                                                        1.0f, md->minVal, md->maxVal);
                        pmd::FeatureState fs;
                        fs.isExtended = true;
                        txt = md->valueToString(denorm, fs)
                                  .value_or(juce::String(denorm).toStdString());
                    }

                    textEditor->setText(txt, juce::dontSendNotification);

                    const auto valCol = juce::Colour(0xFFFF9000);

                    textEditor->setColour(juce::TextEditor::ColourIds::backgroundColourId,
                                          valCol.withAlpha(0.1f));
                    textEditor->setColour(juce::TextEditor::ColourIds::highlightColourId,
                                          valCol.withAlpha(0.15f));
                    textEditor->setColour(juce::TextEditor::ColourIds::outlineColourId,
                                          juce::Colours::black.withAlpha(0.f));
                    textEditor->setColour(juce::TextEditor::ColourIds::focusedOutlineColourId,
                                          juce::Colours::black.withAlpha(0.f));
                    textEditor->setBorder(juce::BorderSize<int>(3));
                    textEditor->applyColourToAllText(valCol, true);
                    textEditor->grabKeyboardFocus();
                    textEditor->selectAll();
                }
            });
        }

        void textEditorReturnKeyPressed(juce::TextEditor &) override
        {
            if (knob)
            {
                DBG("Here we can see if the ParamInfo for this param has a meta and invert it");

                auto md = knob->getMetadata();
                double v{0.f};

                if (md.has_value())
                {
                    std::string em;
                    pmd::FeatureState fs;
                    fs.isExtended = true;
                    auto vv = md->valueFromString(textEditor->getText().toStdString(), em, fs);
                    if (!vv.has_value())
                    {
                        DBG(em);
                    }
                    else
                    {
                        v = *vv;
                        v = juce::jmap(static_cast<float>(v), md->minVal, md->maxVal, 0.0f, 1.0f);
                    }
                }
                else
                {
                    v = textEditor->getText().getDoubleValue();
                }

                knob->setValue(juce::jlimit(knob->getMinimum(), knob->getMaximum(), v),
                               juce::sendNotificationAsync);

                triggerMenuItem();
                juce::MessageManager::callAsync([w = juce::Component::SafePointer(knob)]() {
                    if (w)
                        w->grabKeyboardFocus();
                });
            }
        }

        void textEditorEscapeKeyPressed(juce::TextEditor &) override
        {
            triggerMenuItem();
            juce::MessageManager::callAsync([w = juce::Component::SafePointer(knob)]() {
                if (w)
                    w->grabKeyboardFocus();
            });
        }
    };

  public:
    Knob(juce::String name, const int fh, ObxfAudioProcessor *owner_, ScalingImageCache &cache)
        : Slider("Knob"), img_name(std::move(name)), imageCache(cache), owner(owner_)
    {
        scaleFactorChanged();
        setWantsKeyboardFocus(true);

        h2 = fh;
        w2 = kni.getWidth();
        numFr = kni.getHeight() / h2;

        setLookAndFeel(&lookAndFeel);
        setVelocityModeParameters(0.1, 1, 0.1, true, juce::ModifierKeys::shiftModifier);
    }

    ~Knob() override { setLookAndFeel(nullptr); }

    juce::AudioProcessorParameterWithID *getParameterWithID() override { return parameter; }

    enum SvgTranslationMode
    {
        ROTATION,
        HORIZONTAL,
        VERTICAL
    } svgTranslationMode{ROTATION};

    std::optional<sst::basic_blocks::params::ParamMetaData> getMetadata()
    {
        auto op = dynamic_cast<ObxfParameterFloat *>(parameter);

        if (op)
            return op->meta;
        else
            return std::nullopt;
    }

    void scaleFactorChanged() override
    {
        isSVG = imageCache.isSVG(img_name.toStdString());

        if (!isSVG)
        {
            kni = imageCache.getImageFor(img_name.toStdString(), getWidth(), getHeight());
        }

        repaint();
    }

    void resized() override { scaleFactorChanged(); }

    void showPopupMenu()
    {
        using namespace sst::plugininfra::misc_platform;

        juce::PopupMenu menu;
        auto safeThis = SafePointer(this);

        auto md = getMetadata();

        menu.addSectionHeader(parameter->getName(128));

        menu.addSeparator();

        menu.addCustomItem(-1, std::make_unique<MenuValueTypein>(*owner, this), nullptr,
                           toOSCase("Set Value..."));

        menu.addSeparator();

        menu.addItem(toOSCase("Reset to Default"), [safeThis]() {
            if (safeThis && safeThis->parameter)
                safeThis->setValue(safeThis->parameter->getDefaultValue(),
                                   juce::sendNotificationAsync);
        });

        if (isPanKnob())
        {
            menu.addItem(toOSCase("Reset All Pans to Default"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::RESET_ALL);
            });

            menu.addSeparator();

            menu.addItem(toOSCase("Stereo Spread Narrow"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::SPREAD_25);
            });

            menu.addItem(toOSCase("Stereo Spread Medium"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::SPREAD_50);
            });

            menu.addItem(toOSCase("Stereo Spread Wide"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::SPREAD_100);
            });

            menu.addItem(toOSCase("Alternate Pans Narrow"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::ALTERNATE_25);
            });

            menu.addItem(toOSCase("Alternate Pans Medium"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::ALTERNATE_50);
            });

            menu.addItem(toOSCase("Alternate Pans Wide"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::ALTERNATE_100);
            });

            menu.addItem(toOSCase("Randomize Pans"), [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->panSetter(PanAlgos::RANDOMIZE);
            });
        }

        auto editor = owner->getActiveEditor();

        if (editor)
        {
            if (std::strcmp(juce::PluginHostType().getHostDescription(), "Unknown") != 0)
            {
                if (auto *c = editor->getHostContext())
                {
                    if (auto menuInfo = c->getContextMenuForParameter(parameter))
                    {
                        auto hostMenu = menuInfo->getEquivalentPopupMenu();

                        hostMenu = modifyHostMenu(hostMenu);

                        // merge host menu with our usual context menu
                        if (hostMenu.getNumItems() > 0)
                        {
                            menu.addSeparator();

                            juce::PopupMenu::MenuItemIterator iterator(hostMenu);

                            while (iterator.next())
                            {
                                menu.addItem(iterator.getItem());
                            }
                        }
                    }
                }
            }
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(getTopLevelComponent()));
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

            Slider::mouseDown(event);
        }
    }

    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &d) override
    {
        // juce::Slider provides the wheel support ut we want shift to be slower so
        // whack the event before we call it.
        if (e.mods.isShiftDown())
        {
            auto dcopy = d;
            // juce internally uses abs(dx) > abs(dy) to determine stuff so
            // just force the dy motion for this shift wheel case here
            dcopy.deltaY *= .1;
            dcopy.deltaX = 0.;
            juce::Slider::mouseWheelMove(e, dcopy);
        }
        else
        {
            juce::Slider::mouseWheelMove(e, d);
        }
    }

    juce::String getTextFromValue(double value) override
    {
        if (auto *op = dynamic_cast<ObxfParameterFloat *>(parameter))
        {
            return op->stringFromValue(static_cast<float>(op->denormalizedValue(value)), 0).c_str();
        }

        return juce::String(value);
    }

    double getValueFromText(const juce::String &text) override
    {
        if (auto *op = dynamic_cast<ObxfParameterFloat *>(parameter))
        {
            return op->valueFromString(text.toStdString());
        }

        return text.getDoubleValue();
    }

    void mouseDrag(const juce::MouseEvent &event) override
    {
        Slider::mouseDrag(event);

        if (event.mods.isCommandDown())
        {
            if (cmdDragCallback)
            {
                setValue(cmdDragCallback(getValue()), juce::sendNotificationAsync);
            }
        }
        if (event.mods.isAltDown())
        {
            if (altDragCallback)
            {
                setValue(altDragCallback(getValue()), juce::sendNotificationAsync);
            }
        }
        if (alternativeValueMapCallback)
        {
            setValue(alternativeValueMapCallback(getValue()), juce::sendNotificationAsync);
        }
    }

    void setParameter(juce::AudioProcessorParameterWithID *p)
    {
        if (parameter == p)
            return;

        parameter = p;
        updateText();
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        if (isSVG)
        {
            auto &l0 = imageCache.getSVGDrawable(img_name.toStdString(), 0);
            auto v01 =
                static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
            auto vpm1 = 2 * v01 - 1;
            auto ang = vpm1 * juce::MathConstants<float>::pi * 0.75;

            juce::AffineTransform baseXF, l1XF;

            switch (svgTranslationMode)
            {
            case ROTATION:
            {
                auto kscale = std::min(getWidth(), getHeight()) * 1.f / l0->getWidth();
                baseXF = juce::AffineTransform().scaled(kscale);

                l1XF = baseXF.translated(-getWidth() / 2.f, -getHeight() / 2.f)
                           .rotated(ang)
                           .translated(getWidth() / 2.f, getHeight() / 2.f);
                break;
            }
            case HORIZONTAL:
            {
                auto kscale = getWidth() * 1.f / l0->getWidth();
                baseXF = juce::AffineTransform().scaled(kscale);

                auto l2 = h2 * kscale;
                l1XF = baseXF.translated(v01 * (getWidth() - l2), 0.f);
                break;
            }
            case VERTICAL:
            {
                auto kscale = getHeight() * 1.f / l0->getHeight();
                baseXF = juce::AffineTransform().scaled(kscale);

                auto l2 = h2 * kscale;
                l1XF = baseXF.translated(0.f, -(v01 * (getHeight() - l2)));
                break;
            }
            }

            if (imageCache.getSvgLayerCount(img_name.toStdString()) == 1)
            {
                l0->draw(g, 1.f, l1XF);
            }
            else
            {
                l0->draw(g, 1.f, baseXF);
            }
            if (imageCache.getSvgLayerCount(img_name.toStdString()) > 1)
            {
                auto &l1 = imageCache.getSVGDrawable(img_name.toStdString(), 1);

                l1->draw(g, 1.f, l1XF);
            }
            if (imageCache.getSvgLayerCount(img_name.toStdString()) > 2)
            {
                auto &l2 = imageCache.getSVGDrawable(img_name.toStdString(), 2);

                l2->draw(g, 1.f, baseXF);
            }
        }
        else
        {
            const int ofs = static_cast<int>((getValue() - getMinimum()) /
                                             (getMaximum() - getMinimum()) * (numFr - 1));

            const int zoomLevel =
                imageCache.zoomLevelFor(img_name.toStdString(), getWidth(), getHeight());
            constexpr int baseZoomLevel = 100;

            const float scale = static_cast<float>(zoomLevel) / static_cast<float>(baseZoomLevel);

            const int srcW = w2 * scale;
            const int srcH = h2 * scale;
            const int srcY = h2 * ofs * scale;

            g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, srcY, srcW, srcH);
        }
    }

    std::function<double(double)> cmdDragCallback, altDragCallback, alternativeValueMapCallback;

    bool keyPressed(const juce::KeyPress &e) override
    {
        if (e.getModifiers().isShiftDown() && e.getKeyCode() == juce::KeyPress::F10Key)
        {
            showPopupMenu();
            return true;
        }

        auto obp = dynamic_cast<ObxfParameterFloat *>(parameter);

        if (obp)
        {
            if (e.getKeyCode() == juce::KeyPress::homeKey)
            {
                setValue(obp->convertTo0to1(obp->meta.maxVal), juce::sendNotificationAsync);
                return true;
            }

            if (e.getKeyCode() == juce::KeyPress::endKey)
            {
                setValue(obp->convertTo0to1(obp->meta.minVal), juce::sendNotificationAsync);
                return true;
            }

            if (e.getKeyCode() == juce::KeyPress::deleteKey ||
                e.getKeyCode() == juce::KeyPress::backspaceKey)
            {
                setValue(obp->convertTo0to1(obp->meta.defaultVal), juce::sendNotificationAsync);
                return true;
            }
        }

        return juce::Slider::keyPressed(e);
    }

  private:
    bool isPanKnob() const
    {
        auto param = dynamic_cast<ObxfParameterFloat *>(parameter);

        if (param)
        {
            if (param->meta.hasFeature(IS_PAN))
            {
                return true;
            }
        }

        return false;
    }

    juce::PopupMenu modifyHostMenu(juce::PopupMenu menu)
    {
        // make things look a bit nicer for our friends from Image-Line
        if (juce::PluginHostType().isFruityLoops())
        {
            auto it = juce::PopupMenu::MenuItemIterator(menu);

            while (it.next())
            {
                auto txt = it.getItem().text;

                if (txt.startsWithChar('-'))
                {
                    it.getItem().isSectionHeader = true;
                    it.getItem().text = txt.fromFirstOccurrenceOf("-", false, false);
                }
            }

            return menu;
        }

        // we really don't need that parameter name repeated in Reaper...
        if (juce::PluginHostType().isReaper())
        {
            auto newMenu = juce::PopupMenu();
            auto it = juce::PopupMenu::MenuItemIterator(menu);

            while (it.next())
            {
                auto txt = it.getItem().text;
                bool include = true;

                if (txt.startsWithChar('[') && txt.endsWithChar(']'))
                {
                    include = it.next();
                }

                if (include)
                {
                    newMenu.addItem(it.getItem());
                }
            }

            return newMenu;
        }

        return menu;
    }

    bool isSVG{false};

    juce::Image kni;

    int numFr;
    int w2, h2;
    juce::AudioProcessorParameterWithID *parameter{nullptr};
    KnobLookAndFeel lookAndFeel;
    juce::AudioProcessor *owner{nullptr};
};

#endif // OBXF_SRC_GUI_KNOB_H
