/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#if HAS_CLAP_JUCE_EXTENSIONS

#include <cstring>
#include <string>

#include <clap/clap.h>

#include "Utils.h"
#include "StateManager.h"
#include "ObxdImporter.h"

namespace obxf::preset_discovery
{
static constexpr const char *pluginClapId{"org.surge-synth-team.OB-Xf"};

static const clap_preset_discovery_provider_descriptor desc{
    CLAP_VERSION, "org.surge-synth-team.OB-Xf.preset-indexer", "OB-Xf Patches", "Surge Synth Team"};

struct PresetProvider
{
    const clap_preset_discovery_indexer *indexer{nullptr};
    clap_preset_discovery_provider provider{};

    explicit PresetProvider(const clap_preset_discovery_indexer *idx) : indexer(idx)
    {
        provider.desc = &desc;
        provider.provider_data = this;
        provider.init = [](auto *p) { return fromProvider(p)->init(); };
        provider.destroy = [](auto *p) { delete fromProvider(p); };
        provider.get_metadata = [](auto *p, auto kind, const auto *loc, const auto *rcv) {
            return fromProvider(p)->getMetadata(kind, loc, rcv);
        };
        provider.get_extension = [](auto *, auto *) -> const void * { return nullptr; };
    }

    static PresetProvider *fromProvider(const clap_preset_discovery_provider *p)
    {
        return static_cast<PresetProvider *>(p->provider_data);
    }

    bool declareFolder(const juce::File &folder, uint32_t flags, const char *name)
    {
        if (!folder.isDirectory())
            return true;

        auto path = folder.getFullPathName().toStdString();
        auto loc = clap_preset_discovery_location{flags, name, CLAP_PRESET_DISCOVERY_LOCATION_FILE,
                                                  path.c_str()};
        return indexer->declare_location(indexer, &loc);
    }

    bool init()
    {
        auto fxp = clap_preset_discovery_filetype{"OB-Xf Patch", "", "fxp"};
        auto res = indexer->declare_filetype(indexer, &fxp);

        // utils is not constructed here since that creates the user folders
        res =
            res && declareFolder(Utils::getPatchFolderFor(Utils::SYSTEM_FACTORY),
                                 CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT, "OB-Xf Factory Patches");
        res = res && declareFolder(Utils::getPatchFolderFor(Utils::LOCAL_FACTORY),
                                   CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
                                   "OB-Xf Local Factory Patches");
        res = res && declareFolder(Utils::getPatchFolderFor(Utils::USER),
                                   CLAP_PRESET_DISCOVERY_IS_USER_CONTENT, "OB-Xf User Patches");
        return res;
    }

    bool getMetadata(uint32_t kind, const char *location,
                     const clap_preset_discovery_metadata_receiver *rcv)
    {
        if (!rcv)
            return false;

        auto bail = [rcv, location](const std::string &msg) {
            auto m = std::string(location ? location : "(null)") + ": " + msg;
            rcv->on_error(rcv, 0, m.c_str());
            return false;
        };

        if (kind != CLAP_PRESET_DISCOVERY_LOCATION_FILE || !location)
            return bail("only file locations are supported");

        auto file = juce::File(juce::String::fromUTF8(location));
        juce::MemoryBlock mb;
        if (!file.loadFileAsData(mb))
            return bail("unable to read file");

        auto name = file.getFileNameWithoutExtension();
        std::unique_ptr<juce::XmlElement> xml;

        const void *chunk{nullptr};
        int chunkSize{0};
        if (StateManager::getChunkFromFxpData(mb.getData(), mb.getSize(), chunk, chunkSize))
        {
            xml = juce::AudioProcessor::getXmlFromBinary(chunk, chunkSize);
            if (!xml)
                return bail("patch does not contain valid XML");
            name = xml->getStringAttribute("programName", name);
        }
        else if (!ObxdImporter::isOBXdData(mb.getData(), mb.getSize()))
        {
            return bail("not an OB-Xf patch");
        }

        if (!rcv->begin_preset(rcv, name.toRawUTF8(), ""))
            return bail("unable to begin preset");

        auto pid = clap_universal_plugin_id{"clap", pluginClapId};
        rcv->add_plugin_id(rcv, &pid);

        if (!xml)
            return true;

        auto addInfo = [&](const char *attr, auto &&fn) {
            auto v = xml->getStringAttribute(attr);
            if (v.isNotEmpty())
                fn(v.toRawUTF8());
        };
        addInfo("author", [rcv](auto *v) { rcv->add_creator(rcv, v); });
        addInfo("license", [rcv](auto *v) { rcv->add_extra_info(rcv, "license", v); });
        addInfo("category", [rcv](auto *v) { rcv->add_extra_info(rcv, "category", v); });
        addInfo("project", [rcv](auto *v) { rcv->add_extra_info(rcv, "project", v); });

        return true;
    }
};

static uint32_t count(const clap_preset_discovery_factory *) { return 1; }

static const clap_preset_discovery_provider_descriptor *
getDescriptor(const clap_preset_discovery_factory *, uint32_t index)
{
    return index == 0 ? &desc : nullptr;
}

static const clap_preset_discovery_provider *create(const clap_preset_discovery_factory *,
                                                    const clap_preset_discovery_indexer *indexer,
                                                    const char *providerId)
{
    if (strcmp(providerId, desc.id) != 0)
        return nullptr;

    auto *res = new PresetProvider(indexer);
    return &res->provider;
}

static const clap_preset_discovery_factory factory{count, getDescriptor, create};
} // namespace obxf::preset_discovery

const void *JUCE_CALLTYPE clapJuceExtensionCustomFactory(const char *factoryId)
{
    if (strcmp(factoryId, CLAP_PRESET_DISCOVERY_FACTORY_ID) == 0 ||
        strcmp(factoryId, CLAP_PRESET_DISCOVERY_FACTORY_ID_COMPAT) == 0)
    {
        return &obxf::preset_discovery::factory;
    }

    return nullptr;
}

#endif
