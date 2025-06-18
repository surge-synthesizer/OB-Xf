
import math;

def atsim(atk):
    Value = 0.
    coeff = 0.;
    sr = 48000;

    ts = 0.001
    te = 1.0  # set this to 1 for no acceleration
    twhile = 0.1 # set this to ts for exact calculation
    top = (math.log(ts) - math.log(te))
    bot = sr * atk / 1000;
    coef = top / bot;

    Value = 0.

    idx = 0
    while (Value - 1 < -twhile):
        Value = Value - (1-Value)*coef
        idx = idx + 1

    print(atk, "ms -> ", idx / sr * 1000, "ms; ratio", idx / sr * 1000 / atk, " or ", math.log(twhile)/math.log(ts))

atsim(6)
atsim(70)
atsim(1000)
atsim(4000)
atsim(8000)
atsim(60000)