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

#ifndef OBXF_SRC_STATE_OBXDIMPORTER_H
#define OBXF_SRC_STATE_OBXDIMPORTER_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Program.h"

class ObxdImporter
{
  public:
    // Returns true when the bytes look like an OB-Xd FXP/FXB wrapper
    // (CcnK + fxID == "Obxd"). Does not try to parse the inner XML.
    static bool isOBXdData(const void *data, size_t size);

    // Imports an OB-Xd FXP (single program) onto `outProgram`. For an FXB
    // the program at the file's `currentProgram` index is written.
    // Returns true on success and appends any diagnostics to `outWarnings`.
    static bool importSingleOnto(const void *data, size_t size, Program &outProgram,
                                 std::vector<std::string> *outWarnings = nullptr);

    // Bank visitor. Calls `visit(idx, isCurrent, xmlElement)` for each program
    // in an FXB (or once with idx=0 for an FPCh). The visitor is responsible
    // for translating each XML element via translateProgramFromXml.
    // Returns false when the bytes are not OB-Xd or the wrapper is malformed.
    static bool visitPrograms(
        const void *data, size_t size,
        const std::function<void(int idx, bool isCurrent, const juce::XmlElement &)> &visit);

    // Translate an `<discoDSP>` XML element (single-program form, with
    // Val_<k>/<k> attributes) onto an OB-Xf Program. Public for tests.
    static void translateProgramFromXml(const juce::XmlElement &e, Program &outProgram,
                                        std::vector<std::string> &warnings);
};

#endif // OBXF_SRC_STATE_OBXDIMPORTER_H
