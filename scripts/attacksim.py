import math;

def atsim(atk):
    print("---------")
    Value = 0.
    coeff = 0.;
    sr = 48000;

    ts = 0.001
    te = 1.3  # set this to 1 for no acceleration
    twhile = 0.1 # set this to ts for exact calculation
    top = (math.log(ts) - math.log(te))
    bot = sr * atk / 1000;
    coef = top / bot;

    exprat = math.log(twhile) / math.log(ts)
    exptime = atk * exprat
    linsamp = (1.0 - exprat * twhile) * exptime * sr / 1000;
    dlin = (1-twhile) / linsamp


    Value = 0.
    LValue = 0

    idx = 0
    volv = 0
    while (Value - 1 < -twhile):
        Value = Value - (1-Value)*coef
        LValue = LValue + dlin
        volv = volv + Value / LValue;
        idx = idx + 1
    print ("LinSamples are ", linsamp, " Realized samples are ", idx, "  Ratio: ", idx / linsamp)

    print("LinValue=", LValue, " ExpValue=", Value, "  Ratio: ", LValue / Value)
    print(atk, "ms -> ", idx / sr * 1000, "ms; ratio", idx / sr * 1000 / atk, " or ", math.log(twhile)/math.log(ts))
    print("Average VOLV is ", volv / idx, "")

atsim(6)
atsim(70)
atsim(1000)
atsim(4000)
atsim(8000)
atsim(60000)