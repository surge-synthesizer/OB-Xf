//
// Created by Paul Walker on 11/16/25.
//

#ifndef OB_XF_CONFIGURATION_H
#define OB_XF_CONFIGURATION_H

static constexpr uint64_t currentStreamingVersion{0x2025'11'16};

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
static constexpr bool patches{true};
static constexpr bool state{true};
static constexpr bool rework{true};
static constexpr bool themes{false};
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
#define OBLOG(cond, ...)                                                                           \
    if constexpr (obxf_log::cond)                                                                  \
    {                                                                                              \
        std::cout << srcPath(__FILE__) << ":" << __LINE__ << " [" << #cond << "] " << __VA_ARGS__  \
                  << std::endl;                                                                    \
    }

#define OBD(x) #x << "=" << x << " "

#endif // OB_XF_CONFIGURATION_H
