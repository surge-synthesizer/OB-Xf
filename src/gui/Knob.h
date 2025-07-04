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
#include "../components/ScalableComponent.h"

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

class Knob final : public juce::Slider, public ScalableComponent, public juce::ActionBroadcaster
{
    juce::String img_name;

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
    Knob(juce::String name, const int fh, ObxfAudioProcessor *owner_)
        : Slider("Knob"), ScalableComponent(owner_), img_name(std::move(name)), owner(owner_)
    {
        scaleFactorChanged();

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

    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.mods.isRightButtonDown())
        {
            juce::PopupMenu menu;
            auto safeThis = SafePointer(this);

            auto md = getMetadata();

            menu.addSectionHeader(parameter->getName(128));
            menu.addSeparator();

            menu.addCustomItem(-1, std::make_unique<MenuValueTypein>(this), nullptr,
                               "Set Value...");

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
                    if (const auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
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

            menu.showMenuAsync(
                juce::PopupMenu::Options().withParentComponent(getTopLevelComponent()));
        }
        else
        {
            Slider::mouseDown(event);
        }
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

    // Source:
    // https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
  public:
    class KnobAttachment final : public juce::AudioProcessorValueTreeState::SliderAttachment
    {
        juce::RangedAudioParameter *parameter = nullptr;
        Knob *sliderToControl = nullptr;

      public:
        KnobAttachment(juce::AudioProcessorValueTreeState &stateToControl,
                       const juce::String &parameterID, Knob &sliderToControl)
            : juce::AudioProcessorValueTreeState::SliderAttachment(stateToControl, parameterID,
                                                                   sliderToControl),
              sliderToControl(&sliderToControl)
        {
            parameter = stateToControl.getParameter(parameterID);
            sliderToControl.setParameter(parameter);
        }

        void updateToSlider() const
        {
            const float val = parameter->getValue();
            sliderToControl->setValue(val, juce::NotificationType::dontSendNotification);
        }

        ~KnobAttachment() = default;
    };

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
        const int ofs = static_cast<int>((getValue() - getMinimum()) /
                                         (getMaximum() - getMinimum()) * (numFr - 1));
        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, h2 * ofs, w2, h2);
    }

    std::function<double(double)> cmdDragCallback, altDragCallback, alternativeValueMapCallback;

  private:
    bool isPanKnob() const
    {
        if (parameter)
        {
            const auto name = parameter->getName(5).toLowerCase();
            return name.contains("pan");
        }
        return false;
    }

    juce::Image kni;
    int numFr;
    int w2, h2;
    juce::AudioProcessorParameterWithID *parameter{nullptr};
    KnobLookAndFeel lookAndFeel;
    juce::AudioProcessor *owner{nullptr};
};

#endif // OBXF_SRC_GUI_KNOB_H