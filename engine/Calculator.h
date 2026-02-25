#pragma once
#include "CableData.h"
#include <cmath>
#include <string>

enum class Arrangement { TrefoilTouching, FlatTouching, FlatSpaced };

struct SystemParams {
    double      voltageKV   = 33.0;
    double      powerMVA    = 10.0;
    double      powerFactor = 0.95;
    double      lengthKm    = 1.0;
    Arrangement arrangement = Arrangement::TrefoilTouching;
    int         sizeMm2     = 240;
};

struct CalcResults {
    double R           = 0.0;
    double X           = 0.0;
    double Z           = 0.0;
    double current     = 0.0;
    double deltaV_V    = 0.0;
    double deltaV_pct  = 0.0;
    double P_MW        = 0.0;
    double Q_Mvar      = 0.0;
    double losses_kW   = 0.0;
    double dielLoss_kW = 0.0;
    double losses_pct  = 0.0;
    double chargingA   = 0.0;
};

// cable is looked up by the caller via DatabaseManager and passed in.
inline CalcResults calculate(const SystemParams& p, const CableRecord& cable)
{
    if (cable.sizeMm2 == 0) return {};

    double R_per_km = 0.0, X_per_km = 0.0;
    switch (p.arrangement) {
        case Arrangement::TrefoilTouching:
            R_per_km = cable.acResistanceTrefoilTouching;
            X_per_km = cable.inductiveReactanceTrefoilTouching;
            break;
        case Arrangement::FlatTouching:
            R_per_km = cable.acResistanceFlatTouching;
            X_per_km = cable.inductiveReactanceFlatTouching;
            break;
        case Arrangement::FlatSpaced:
            R_per_km = (cable.acResistanceFlatSpaced > 0)
                       ? cable.acResistanceFlatSpaced
                       : cable.acResistanceFlatTouching;
            X_per_km = cable.inductiveReactanceFlatSpaced;
            break;
    }

    CalcResults r;
    r.R = R_per_km * p.lengthKm;
    r.X = X_per_km * p.lengthKm;
    r.Z = std::sqrt(r.R * r.R + r.X * r.X);

    const double Vph    = (p.voltageKV * 1000.0) / std::sqrt(3.0);
    r.current           = (p.powerMVA * 1e6) / (std::sqrt(3.0) * p.voltageKV * 1000.0);

    const double sinPhi = std::sqrt(std::max(0.0, 1.0 - p.powerFactor * p.powerFactor));
    const double dVph   = r.current * (r.R * p.powerFactor + r.X * sinPhi);
    r.deltaV_V          = dVph * std::sqrt(3.0);
    r.deltaV_pct        = (dVph / Vph) * 100.0;

    r.P_MW              = p.powerMVA * p.powerFactor;
    r.Q_Mvar            = p.powerMVA * sinPhi;
    r.losses_kW         = 3.0 * r.current * r.current * r.R / 1000.0;
    r.dielLoss_kW       = cable.dielectricLossPerPhase * p.lengthKm * 3.0 / 1000.0;
    r.losses_pct        = ((r.losses_kW) / (r.P_MW * 1e3)) * 100;
    r.chargingA         = cable.chargingCurrentPerPhase * p.lengthKm;

    return r;
}
