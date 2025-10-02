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

#ifndef OBXF_SRC_ENGINE_BANK_H
#define OBXF_SRC_ENGINE_BANK_H

#include <cassert>

#include "Constants.h"
#include "Voice.h"

class Bank
{
  public:
    Program programs[MAX_PROGRAMS], originalPrograms[MAX_PROGRAMS];
    std::atomic<int> currentProgram{0};
    std::array<bool, MAX_PROGRAMS> programDirty{};

    Bank() = default;

    Program &getCurrentProgram()
    {
        assert(hasCurrentProgram());
        return programs[currentProgram.load()];
    }
    const Program &getCurrentProgram() const
    {
        assert(hasCurrentProgram());
        return programs[currentProgram.load()];
    }

    void setCurrentProgramDirty(bool isDirty)
    {
        assert(hasCurrentProgram());
        if (hasCurrentProgram())
            programDirty[currentProgram.load()] = isDirty;
    }

    bool getIsCurrentProgramDirty() const
    {
        assert(hasCurrentProgram());
        return hasCurrentProgram() && programDirty[currentProgram.load()];
    }

    bool hasCurrentProgram() const
    {
        int idx = currentProgram.load();
        return idx >= 0 && idx < MAX_PROGRAMS;
    }

    void setCurrentProgram(int idx)
    {
        if (idx >= 0 && idx < MAX_PROGRAMS)
            currentProgram.store(idx);
    }
};

#endif // OBXF_SRC_ENGINE_BANK_H