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

#ifndef OB_XF_SRC_GUI_FOCUSDEBUGGER_H
#define OB_XF_SRC_GUI_FOCUSDEBUGGER_H

#include <juce_gui_basics/juce_gui_basics.h>

struct FocusDebugger : public juce::FocusChangeListener
{
    juce::Component &debugInto;
    FocusDebugger(juce::Component &c) : debugInto(c)
    {
        juce::Desktop::getInstance().addFocusChangeListener(this);
    }
    ~FocusDebugger() { juce::Desktop::getInstance().removeFocusChangeListener(this); }
    void setDoFocusDebug(bool fd)
    {
        doFocusDebug = fd;
        if (doFocusDebug)
            guaranteeDebugComponent();
        if (debugComponent)
            debugComponent->setVisible(fd);
    }

    bool doFocusDebug{false};
    void globalFocusChanged(juce::Component *fc) override
    {
        if (!doFocusDebug)
            return;

        if (!fc)
            return;
        auto ofc = fc;
        guaranteeDebugComponent();
        debugComponent->toFront(false);
        auto bd = fc->getBounds();
        fc = fc->getParentComponent();
        while (fc && fc != &debugInto)
        {
            bd += fc->getBounds().getTopLeft();
            fc = fc->getParentComponent();
        }

        std::cout << "FD : [" << std::hex << ofc << std::dec << "] " << ofc->getTitle() << " @ "
                  << bd.toString() << " explicitFocusOrder=" << ofc->getExplicitFocusOrder() << " "
                  << ofc->getExplicitFocusOrder() << " " << typeid(*ofc).name() << std::endl;
        debugComponent->setBounds(bd);
    }

    struct DbgComponent : juce::Component
    {
        DbgComponent()
        {
            setAccessible(false);
            setWantsKeyboardFocus(false);
            setMouseClickGrabsKeyboardFocus(false);
            setInterceptsMouseClicks(false, false);
            setTitle("Debug Component");
        }

        void paint(juce::Graphics &g) override
        {
            g.fillAll(juce::Colours::red.withAlpha(0.1f));
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds(), 1);
        }
    };

    void guaranteeDebugComponent()
    {
        if (!debugComponent)
        {
            debugComponent = std::make_unique<DbgComponent>();
            debugInto.addAndMakeVisible(*debugComponent);
        }
    }
    std::unique_ptr<juce::Component> debugComponent;
};
#endif // OB_XF_FOCUSDEBUGGER_H
