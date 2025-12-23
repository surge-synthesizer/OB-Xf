//
// Created by Paul Walker on 11/16/25.
//

#ifndef OB_XF_CONFIGURATION_H
#define OB_XF_CONFIGURATION_H

static constexpr uint64_t currentStreamingVersion{0x2025'12'13};

constexpr int MAX_VOICES{32};
constexpr int MAX_PROGRAMS{256};
constexpr int MAX_BEND_RANGE{48};
constexpr int MAX_PANNINGS{8};
constexpr int MAX_UNISON{8};
constexpr uint8_t NUM_PATCHES_PER_GROUP{16};
constexpr uint8_t NUM_LFOS{2};
constexpr uint8_t NUM_XPANDER_MODES{15};
constexpr uint8_t OVERSAMPLE_FACTOR{2};

constexpr int fxbVersionNum = 1;

static const std::string INIT_PATCH_NAME{"Init"};

namespace obxf_log
{
static constexpr bool general{true};
static constexpr bool patches{false};
static constexpr bool patchSave{false};
static constexpr bool state{false};
static constexpr bool themes{false};
static constexpr bool undo{false};
static constexpr bool params{false};
static constexpr bool paramSet{false};
static constexpr bool midiLearn{false};
} // namespace obxf_log

// TODO - grab more fulsome version from ShortCircuit
inline std::string srcPath(const std::string &s)
{
    auto sl = s.find("src/");
    if (sl == std::string::npos)
        sl = s.find("src\\");
    if (sl == std::string::npos)
        return s;
    return s.substr(sl);
}

#define OBLOG_TO_FILE 0
#if OBLOG_TO_FILE
// so please leave this in until we are sure we have it all OK?
#include <fstream>
extern std::ofstream logHack;
#define OBLOG(cond, ...)                                                                           \
    if constexpr (obxf_log::cond)                                                                  \
    {                                                                                              \
        std::cout << srcPath(__FILE__) << ":" << __LINE__ << " [" << #cond << "] " << __VA_ARGS__  \
                  << std::endl;                                                                    \
        if (!logHack.is_open())                                                                    \
            logHack = std::ofstream("/tmp/obxf-log.txt");                                          \
        logHack << srcPath(__FILE__) << ":" << __LINE__ << " [" << #cond << "] " << __VA_ARGS__    \
                << std::endl;                                                                      \
    }
#else
#define OBLOG(cond, ...)                                                                           \
    if constexpr (obxf_log::cond)                                                                  \
    {                                                                                              \
        std::cout << srcPath(__FILE__) << ":" << __LINE__ << " [" << #cond << "] " << __VA_ARGS__  \
                  << std::endl;                                                                    \
    }
#endif

#define OBLOGREMOVE(cond) OBLOG(cond, "remove " << __func__)

#define OBLOGONCE(cond, ...)                                                                       \
    if constexpr (obxf_log::cond)                                                                  \
    {                                                                                              \
        static bool x847294149342fofofofo{false};                                                  \
        if (!x847294149342fofofofo)                                                                \
        {                                                                                          \
            x847294149342fofofofo = true;                                                          \
            OBLOG(cond, " -/ONCE\\- " << __VA_ARGS__);                                             \
        }                                                                                          \
    }

#define OBD(x) #x << "=" << x << " "

#endif // OB_XF_CONFIGURATION_H
