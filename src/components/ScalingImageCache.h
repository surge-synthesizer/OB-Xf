#pragma once
#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <map>
#include <string>
#include "Utils.h"

struct ScalingImageCache
{

    explicit ScalingImageCache(Utils &utilsRef);
    juce::Image getImageFor(const std::string &label, int w, int h);
    int zoomLevelFor(const std::string &label, int w, int h);
    void clearCache();
    juce::File skinDir;

  private:
    juce::Image initializeImage(const std::string &label);
    void guaranteeImageFor(const std::string &label, int zoomLevel);
    void setSkinDir();
    Utils &utils;

    std::unordered_map<std::string, std::map<int, juce::File>> cachePaths;
    std::unordered_map<std::string, std::map<int, std::optional<juce::Image>>> cacheImages;
    std::unordered_map<std::string, std::pair<int, int>> cacheSizes;
    static constexpr std::array<int, 3> zoomLevels = {100, 200, 400};
    int baseZoomLevel = 100;
};
