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

class Knob final : public juce::Slider, public juce::ActionBroadcaster
{
    juce::String img_name;
    ScalingImageCache &imageCache;

    struct MenuValueTypein final : juce::PopupMenu::CustomComponent, juce::TextEditor::Listener
    {
        SafePointer<Knob> knob;
        std::unique_ptr<juce::TextEditor> textEditor;

        explicit MenuValueTypein(Knob *k) : juce::PopupMenu::CustomComponent(false), knob(k)
        {
            textEditor = std::make_unique<juce::TextEditor>();
            textEditor->addListener(this);
            textEditor->setWantsKeyboardFocus(true);
            textEditor->setIndents(2, 0);
            textEditor->setCaretVisible(true);
            textEditor->setReadOnly(false);

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

                    const auto valCol = juce::Colour(0xFF, 0x90, 0x00);
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
            }
        }

        void textEditorEscapeKeyPressed(juce::TextEditor &) override { triggerMenuItem(); }
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
        setVelocityModeParameters(0.25, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    }

    ~Knob() override { setLookAndFeel(nullptr); }

    std::optional<sst::basic_blocks::params::ParamMetaData> getMetadata()
    {
        auto op = dynamic_cast<ObxfParameterFloat *>(parameter);

        if (op)
            return op->meta;
        else
            return std::nullopt;
    }

    void scaleFactorChanged()
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
        juce::PopupMenu menu;
        auto safeThis = SafePointer(this);

        auto md = getMetadata();

        menu.addSectionHeader(parameter->getName(128));
        menu.addSeparator();

        menu.addCustomItem(-1, std::make_unique<MenuValueTypein>(this), nullptr, "Set Value...");

        menu.addItem("Reset to Default", [safeThis]() {
            if (safeThis && safeThis->parameter)
                safeThis->setValue(safeThis->parameter->getDefaultValue(),
                                   juce::sendNotificationAsync);
        });

        if (isPanKnob())
        {
            menu.addItem("Randomize All Pans", [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->randomizeAllPans();
            });
            menu.addItem("Reset All Pans to Default", [this]() {
                if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                    obxf->resetAllPansToDefault();
            });
        }

        auto editor = owner->getActiveEditor();
        if (editor)
        {
            if (auto *c = editor->getHostContext())
            {
                if (auto menuInfo = c->getContextMenuForParameter(parameter))
                {
                    auto hmen = menuInfo->getEquivalentPopupMenu();

                    menu.addSubMenu("Host Controls", hmen);
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
            Slider::mouseDown(event);
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
            return op->valueFromString(text.toStdString());
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

            auto kscale = std::min(getWidth(), getHeight()) * 1.f / l0->getWidth();
            auto baseXF = juce::AffineTransform().scaled(kscale);
            auto l1XF = baseXF.translated(-getWidth() / 2.f, -getHeight() / 2.f)
                            .rotated(ang)
                            .translated(getWidth() / 2.f, getHeight() / 2.f);

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

  public:
    bool keyPressed(const juce::KeyPress &e) override
    {
        if (e.getModifiers().isShiftDown() || e.getKeyCode() == juce::KeyPress::F10Key)
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
    bool isSVG{false};

    juce::Image kni;

    int numFr;
    int w2, h2;
    juce::AudioProcessorParameterWithID *parameter{nullptr};
    KnobLookAndFeel lookAndFeel;
    juce::AudioProcessor *owner{nullptr};
};

#endif // OBXF_SRC_GUI_KNOB_H
