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

#include "Constants.h"
#include "Voice.h"

class Bank
{
  public:
    Parameters programs[MAX_PROGRAMS];
    std::atomic<Parameters *> currentProgramPtr{};
    std::atomic<int> currentProgram{};
    Bank()
    {
        currentProgram = 0;
        currentProgramPtr = programs + currentProgram;
    }
};

#endif // OBXF_SRC_ENGINE_BANK_H
