#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SheathCalc.hpp
// Sheath voltage rise calculator for single-circuit HV cable routes.
// Theory: IEEE 575-2014 Annex D.
//
// The engine works purely with physical quantities.  It has no knowledge of
// cable OD, "arrangements", or any UI concern.  The caller supplies Sab/Sbc/Sac
// directly — derived from whatever installation geometry the user enters.
// ─────────────────────────────────────────────────────────────────────────────

#include <array>
#include <complex>
#include <string>
#include <vector>

namespace sheath {

// ── One section of the cable route ───────────────────────────────────────────
struct RouteSection {
    double      length_m  = 10.0;  // Section length (metres)
    double      Sab_mm    = 0.0;   // Phase A–B centre-to-centre spacing (mm)
    double      Sbc_mm    = 0.0;   // Phase B–C centre-to-centre spacing (mm)
    double      Sac_mm    = 0.0;   // Phase A–C centre-to-centre spacing (mm)
    bool        transpose = false; // Cross-bond sheath transpose at start of section
    std::string label;             // Free-form description (optional)
};

// ── System parameters ─────────────────────────────────────────────────────────
struct SheathParams {
    double current_A    = 0.0;   // Load current magnitude (A, RMS)
    double frequency_Hz = 50.0;  // Power system frequency (Hz)

    // Formula variant:
    //
    //  SIMPLIFIED — imaginary (inductive) coupling terms only.
    //               Conservative; suitable for preliminary studies.
    //    Epm[A] = jω·Ia·(μ₀/2π)·[−½·ln(2·Sab²/(Sab·Sac))]
    //    Epm[B] = jω·Ib·(μ₀/2π)·[−½·ln(4·Sbc²/Sab²)]
    //    Epm[C] = jω·Ic·(μ₀/2π)·[−½·ln(2·Sbc²/(Sab·Sac))]
    //
    //  FULL — real + imaginary coupling terms (recommended for final design).
    //    Epm[A] = jω·Ia·(μ₀/2π)·[−½·ln(2·Sab²/(Sab·Sac)) + j·(√3/2)·ln(2·Sac/Sab)]
    //    Epm[B] = jω·Ib·(μ₀/2π)·[ ½·ln(4·Sab·Sbc/Sab²)  + j·(√3/2)·ln(Sbc/Sab)]
    //    Epm[C] = jω·Ic·(μ₀/2π)·[−½·ln(2·Sbc²/(Sab·Sac)) − j·(√3/2)·ln(2·Sac/Sab)]
    //
    enum class Formula { SIMPLIFIED, FULL } formula = Formula::FULL;

    // Route sections — must be populated by the caller.
    std::vector<RouteSection> route;
};

// ── Results ────────────────────────────────────────────────────────────────────
struct SheathResults {
    bool   valid       = false;
    int    totalLength = 0;

    // Complex sheath voltage phasor at each metre: [metre][phase 0=A, 1=B, 2=C]
    std::vector<std::array<std::complex<double>, 3>> E;

    // Voltage magnitude at each metre: [metre][phase]
    std::vector<std::array<double, 3>> Emag;

    // Distance from route start (metres) at each cross-bond transpose
    std::vector<int> minorBoundaries;

    // Peak sheath voltage across the entire route, per phase (V)
    double maxVoltage_A = 0.0;
    double maxVoltage_B = 0.0;
    double maxVoltage_C = 0.0;

    std::string errorMsg;
};

// ── Public API ────────────────────────────────────────────────────────────────
SheathResults calculate(const SheathParams& params);

} // namespace sheath
