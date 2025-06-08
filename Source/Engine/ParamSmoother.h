/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_PARAMSMOOTHER_H
#define OBXF_SRC_ENGINE_PARAMSMOOTHER_H
#include "SynthEngine.h"

const float PSSC = 0.0030;
class ParamSmoother
{
  private:
    float steepValue;
    float integralValue;
    float srCor;

  public:
    ParamSmoother()
    {
        steepValue = integralValue = 0;
        srCor = 1;
    };
    float smoothStep()
    {
        integralValue = integralValue + (steepValue - integralValue) * PSSC * srCor + dc;
        return integralValue;
    }
    void setSteep(float value) { steepValue = value; }
    void setSampleRate(float sr) { srCor = sr / 44000; }
};

#endif // OBXF_SRC_ENGINE_PARAMSMOOTHER_H
