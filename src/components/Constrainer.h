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

#ifndef OBXF_SRC_COMPONENTS_CONSTRAINER_H
#define OBXF_SRC_COMPONENTS_CONSTRAINER_H

#include <juce_gui_basics/juce_gui_basics.h>

class AspectRatioDownscaleConstrainer final : public juce::ComponentBoundsConstrainer
{
  public:
    AspectRatioDownscaleConstrainer(const int origWidth, const int origHeight)
        : originalWidth(origWidth), originalHeight(origHeight),
          aspectRatio(static_cast<float>(origWidth) / static_cast<float>(origHeight))
    {
    }

    void checkBounds(juce::Rectangle<int> &bounds, const juce::Rectangle<int> & /*previousBounds*/,
                     const juce::Rectangle<int> & /*limits*/, bool /*isStretchingTop*/,
                     bool /*isStretchingLeft*/, bool /*isStretchingBottom*/,
                     bool /*isStretchingRight*/) override
    {
        const int minWidth = originalWidth / 2;
        const int minHeight = originalHeight / 2;
        const int maxWidth = originalWidth * 2;
        const int maxHeight = originalHeight * 2;

        bounds.setWidth(juce::jmax(minWidth, juce::jmin(bounds.getWidth(), maxWidth)));
        bounds.setHeight(juce::jmax(minHeight, juce::jmin(bounds.getHeight(), maxHeight)));

        if (const float currentRatio =
                static_cast<float>(bounds.getWidth()) / static_cast<float>(bounds.getHeight());
            currentRatio > aspectRatio)
            bounds.setWidth(juce::roundToInt(bounds.getHeight() * aspectRatio));
        else if (currentRatio < aspectRatio)
            bounds.setHeight(juce::roundToInt(bounds.getWidth() / aspectRatio));
    }

  private:
    int originalWidth, originalHeight;
    float aspectRatio;
};
#endif // OBXF_SRC_COMPONENTS_CONTRAINER_H
