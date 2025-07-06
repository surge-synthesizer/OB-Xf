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

#ifndef OBXF_SRC_COMPONENTS_SCALABLECOMPONENT_H
#define OBXF_SRC_COMPONENTS_SCALABLECOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>

class ObxfAudioProcessor;

class ScalableComponent
{
  public:
    virtual ~ScalableComponent();

    virtual void scaleFactorChanged();

    void setOriginalBounds(const juce::Rectangle<int> &bounds) { originalBounds = bounds; }

    juce::Rectangle<int> getOriginalBounds() const { return originalBounds; }

  protected:
    explicit ScalableComponent(ObxfAudioProcessor *owner_);

    juce::Image getScaledImageFromCache(const juce::String &imageName) const;
    juce::Rectangle<int> originalBounds;

  private:
    ObxfAudioProcessor *processor;
};

#endif // OBXF_SRC_COMPONENTS_SCALECOMPONENT_H
