// ─────────────────────────────────────────────────────────────────────────────
// SheathCalc.cpp
// Pure calculation engine — no UI, no arrangement logic, no cable OD.
// ─────────────────────────────────────────────────────────────────────────────
#include "SheathCalc.hpp"

#include <cmath>

namespace sheath {

using cd = std::complex<double>;
static constexpr double PI = 3.14159265358979323846;

// ─────────────────────────────────────────────────────────────────────────────
// calcEpm — induced sheath EMF per metre (complex, V/m)
// All spacing arguments in metres.
// ─────────────────────────────────────────────────────────────────────────────
static std::array<cd, 3> calcEpm(
    cd Ia, cd Ib, cd Ic,
    double Sab, double Sbc, double Sac,
    double f,
    SheathParams::Formula formula)
{
    const double w     = 2.0 * PI * f;
    const double mu    = 2e-7;           // μ₀/(2π)
    const cd     jw    = cd(0.0, w);
    const double sq3_2 = std::sqrt(3.0) / 2.0;

    std::array<cd, 3> Epm{};

    if (formula == SheathParams::Formula::SIMPLIFIED) {
        Epm[0] = jw * Ia * mu * (-0.5 * std::log((2.0*Sab*Sab) / (Sab*Sac)));
        Epm[1] = jw * Ib * mu * (-0.5 * std::log((4.0*Sbc*Sbc) / (Sab*Sab)));
        Epm[2] = jw * Ic * mu * (-0.5 * std::log((2.0*Sbc*Sbc) / (Sab*Sac)));
    } else {
        Epm[0] = jw * Ia * mu * (
            (-0.5 * std::log((2.0*Sab*Sab) / (Sab*Sac)))
            + cd(0.0, sq3_2) * std::log((2.0*Sac) / Sab));

        Epm[1] = jw * Ib * mu * (
            (0.5 * std::log((4.0*Sab*Sbc) / (Sab*Sab)))
            + cd(0.0, sq3_2) * std::log(Sbc / Sab));

        Epm[2] = jw * Ic * mu * (
            (-0.5 * std::log((2.0*Sbc*Sbc) / (Sab*Sac)))
            - cd(0.0, sq3_2) * std::log((2.0*Sac) / Sab));
    }
    return Epm;
}

// ─────────────────────────────────────────────────────────────────────────────
// calculate
// ─────────────────────────────────────────────────────────────────────────────
SheathResults calculate(const SheathParams& params)
{
    SheathResults res;
    const auto& route = params.route;

    // ── Validation ────────────────────────────────────────────────────────────
    if (route.empty()) {
        res.errorMsg = "Route is empty — add at least one section.";
        return res;
    }
    if (params.current_A <= 0.0) {
        res.errorMsg = "Current must be > 0.";
        return res;
    }
    for (size_t i = 0; i < route.size(); ++i) {
        if (route[i].length_m <= 0.0) {
            res.errorMsg = "Section " + std::to_string(i+1) + ": length must be > 0.";
            return res;
        }
        if (route[i].Sab_mm <= 0.0 || route[i].Sbc_mm <= 0.0 || route[i].Sac_mm <= 0.0) {
            res.errorMsg = "Section " + std::to_string(i+1) + ": all spacings must be > 0.";
            return res;
        }
    }

    // ── Setup ─────────────────────────────────────────────────────────────────
    int totalLen = 0;
    for (const auto& sec : route)
        totalLen += static_cast<int>(sec.length_m);
    res.totalLength = totalLen;

    // Current phasors — IEEE 575-2014 Annex D:  a = e^(j2π/3)
    const cd a(-0.5, std::sqrt(3.0) / 2.0);
    const cd I0 = params.current_A;
    cd Ia = a     * I0;
    cd Ib =         I0;
    cd Ic = (a*a) * I0;

    // ── Initialise from section 0 ─────────────────────────────────────────────
    auto Epm = calcEpm(Ia, Ib, Ic,
        route[0].Sab_mm * 1e-3,
        route[0].Sbc_mm * 1e-3,
        route[0].Sac_mm * 1e-3,
        params.frequency_Hz, params.formula);

    res.E   .resize(totalLen);
    res.Emag.resize(totalLen);
    res.E[0] = { Epm[0], Epm[1], Epm[2] };

    // ── March metre by metre ──────────────────────────────────────────────────
    int sectionVal   = 0;
    int nextBoundary = static_cast<int>(route[0].length_m);

    for (int k = 1; k < totalLen; ++k) {
        if (k == nextBoundary) {
            ++sectionVal;
            if (sectionVal >= static_cast<int>(route.size())) break;
            nextBoundary += static_cast<int>(route[sectionVal].length_m);

            if (route[sectionVal].transpose) {
                res.minorBoundaries.push_back(k);
                // Cross-bond: rotate accumulated sheath history  A←C, C←B, B←A
                for (int m = 0; m < k; ++m) {
                    cd tmp      = res.E[m][0];
                    res.E[m][0] = res.E[m][2];
                    res.E[m][2] = res.E[m][1];
                    res.E[m][1] = tmp;
                }
            }

            Epm = calcEpm(Ia, Ib, Ic,
                route[sectionVal].Sab_mm * 1e-3,
                route[sectionVal].Sbc_mm * 1e-3,
                route[sectionVal].Sac_mm * 1e-3,
                params.frequency_Hz, params.formula);
        }

        res.E[k] = {
            res.E[k-1][0] + Epm[0],
            res.E[k-1][1] + Epm[1],
            res.E[k-1][2] + Epm[2]
        };
    }

    // ── Magnitudes and peaks ──────────────────────────────────────────────────
    for (int k = 0; k < totalLen; ++k) {
        res.Emag[k][0] = std::abs(res.E[k][0]);
        res.Emag[k][1] = std::abs(res.E[k][1]);
        res.Emag[k][2] = std::abs(res.E[k][2]);

        if (res.Emag[k][0] > res.maxVoltage_A) res.maxVoltage_A = res.Emag[k][0];
        if (res.Emag[k][1] > res.maxVoltage_B) res.maxVoltage_B = res.Emag[k][1];
        if (res.Emag[k][2] > res.maxVoltage_C) res.maxVoltage_C = res.Emag[k][2];
    }

    res.valid = true;
    return res;
}

} // namespace sheath
