#include "ScalingImageCache.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

constexpr std::array<int, 3> ScalingImageCache::zoomLevels;

ScalingImageCache::ScalingImageCache(Utils &utilsRef) : utils(utilsRef) { setSkinDir(); }

juce::Image ScalingImageCache::getImageFor(const std::string &label, const int w, const int h)
{
    initializeImage(label);
    const int zl = zoomLevelFor(label, w, h);
    guaranteeImageFor(label, zl);
    auto &imgMap = cacheImages[label];

    if (const auto it = imgMap.find(zl); it != imgMap.end() && it->second.has_value())
        return *(it->second);

    if (const auto it100 = imgMap.find(baseZoomLevel);
        it100 != imgMap.end() && it100->second.has_value())
        return *(it100->second);

    return {};
}

void ScalingImageCache::clearCache()
{
    cacheImages.clear();
    cachePaths.clear();
    cacheSizes.clear();
}

juce::Image ScalingImageCache::initializeImage(const std::string &label)
{
    if (cachePaths.find(label) != cachePaths.end())
        return {};

    if (!skinDir.exists())
        return {};

    if (const juce::File basePath = skinDir.getChildFile(label + ".png"); basePath.existsAsFile())
    {
        cachePaths[label][baseZoomLevel] = basePath;
        for (int zl : zoomLevels)
        {
            if (zl == baseZoomLevel)
                continue;

            std::string suffix = fmt::format("@{}x", zl / baseZoomLevel);
            if (juce::File zp = skinDir.getChildFile(label + suffix + ".png"); zp.existsAsFile())
                cachePaths[label][zl] = zp;
        }

        juce::Image img = juce::ImageCache::getFromFile(basePath);
        cacheImages[label][baseZoomLevel] = img;

        if (img.isValid())
            cacheSizes[label] = {img.getWidth(), img.getHeight()};
        return img;
    }
    return {};
}

int ScalingImageCache::zoomLevelFor(const std::string &label, const int w, int /*h*/)
{
    if (cacheSizes.find(label) == cacheSizes.end())
        return baseZoomLevel;

    const double back = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
    auto base = cacheSizes[label];
    const double mu = back * (static_cast<float>(w) / static_cast<float>(base.first));

    for (const int zl : zoomLevels)
    {
        if (zl >= static_cast<int>(mu * baseZoomLevel))
        {
            if (cachePaths[label].find(zl) != cachePaths[label].end())
                return zl;
        }
    }

    for (auto it = cachePaths[label].rbegin(); it != cachePaths[label].rend(); ++it)
    {
        if (it->second.existsAsFile())
            return it->first;
    }

    return baseZoomLevel;
}

void ScalingImageCache::guaranteeImageFor(const std::string &label, const int zoomLevel)
{
    auto &imgMap = cacheImages[label];

    if (imgMap.find(zoomLevel) != imgMap.end())
        return;

    auto &pathMap = cachePaths[label];

    if (const auto it = pathMap.find(zoomLevel); it != pathMap.end())
    {
        const juce::Image img = juce::ImageCache::getFromFile(it->second);
        imgMap[zoomLevel] = img;
    }
}

void ScalingImageCache::setSkinDir()
{
    if (const juce::File theme = utils.getCurrentThemeFolder(); theme.isDirectory())
        skinDir = theme;
}
