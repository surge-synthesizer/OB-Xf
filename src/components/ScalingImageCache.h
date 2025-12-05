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

#ifndef OBXF_SRC_COMPONENTS_SCALINGIMAGECACHE_H
#define OBXF_SRC_COMPONENTS_SCALINGIMAGECACHE_H
#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <map>
#include <string>
#include "Utils.h"

struct ScalingImageCache
{
    explicit ScalingImageCache(Utils &utilsRef);
    bool isSVG(const std::string &label);
    int getSvgLayerCount(const std::string &label);
    std::unique_ptr<juce::Drawable> &getSVGDrawable(const std::string &label, int layer = 0);

    bool hasImageFor(const std::string &label);
    juce::Image getImageFor(const std::string &label, int w, int h);
    int zoomLevelFor(const std::string &label, int w, int h);
    void clearCache();
    juce::File skinDir;
    bool embeddedMode{false};

    // A utility to *always* get from embedded no matter what
    std::unique_ptr<juce::Drawable> getEmbeddedVectorDrawable(const std::string &l);

  private:
    juce::Image initializeImage(const std::string &label);
    void guaranteeImageFor(const std::string &label, int zoomLevel);
    void setSkinDir();
    Utils &utils;

    std::unordered_map<std::string, int> svgLayerCount;
    std::unordered_map<std::string, std::vector<std::unique_ptr<juce::Drawable>>> svgLayers;

    std::unordered_map<std::string, std::map<int, juce::File>> cachePaths;
    std::unordered_map<std::string, std::map<int, std::optional<juce::Image>>> cacheImages;
    std::unordered_map<std::string, std::pair<int, int>> cacheSizes;
    static constexpr std::array<int, 3> zoomLevels = {100, 200, 400};
    static constexpr int baseZoomLevel = 100;
};

#endif // OBXF_SRC_COMPONENTS_SCALINGIMAGECACHE_H
