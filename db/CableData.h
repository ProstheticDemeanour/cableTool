#pragma once
#include <vector>
#include <string>

// ── Plain data struct – all electrical parameters for one conductor size ──────
struct CableRecord {
    int    sizeMm2                              = 0;

    double maxDcResistance20C                   = 0.0;
    double acResistanceTrefoilTouching          = 0.0;
    double acResistanceFlatTouching             = 0.0;
    double acResistanceFlatSpaced               = -1.0;  // -1 = not available
    double inductiveReactanceTrefoilTouching    = 0.0;
    double inductiveReactanceFlatTouching       = 0.0;
    double inductiveReactanceFlatSpaced         = 0.0;
    double insulationResistance20C              = 0.0;
    double conductorToScreenCapacitance         = 0.0;
    double chargingCurrentPerPhase              = 0.0;
    double dielectricLossPerPhase               = 0.0;
    double maxDielectricStress                  = 0.0;
    double screenDcResistance20C                = 0.0;
    double zeroSequenceResistance20C            = 0.0;
    double zeroSequenceReactance50Hz            = 0.0;
};

// ── Static table (mirrors the supplied JSON exactly) ─────────────────────────
inline const std::vector<CableRecord>& cableDatabase()
{
    static const std::vector<CableRecord> db = {
        {   50, 0.387,  0.494,  0.494,  -1,    0.163, 0.178, 0.224, 18000, 0.133, 0.796,  60.5, 4.05, 0.372, 0.759, 0.0999 },
        {   70, 0.268,  0.342,  0.342,  -1,    0.154, 0.169, 0.215, 16000, 0.148, 0.883,  67.1, 3.82, 0.263, 0.531, 0.0919 },
        {   95, 0.193,  0.247,  0.247,  -1,    0.143, 0.158, 0.204, 15000, 0.165, 0.984,  74.8, 3.61, 0.263, 0.457, 0.0817 },
        {  120, 0.153,  0.195,  0.195,  -1,    0.137, 0.153, 0.198, 14000, 0.179, 1.07,   81.1, 3.48, 0.263, 0.416, 0.0767 },
        {  150, 0.124,  0.159,  0.159,  -1,    0.133, 0.148, 0.194, 13000, 0.191, 1.14,   86.8, 3.38, 0.264, 0.369, 0.0731 },
        {  185, 0.0991, 0.127,  0.127,  -1,    0.129, 0.144, 0.190, 12000, 0.205, 1.23,   93.2, 3.29, 0.264, 0.364, 0.0693 },
        {  240, 0.0754, 0.0976, 0.0972, -1,    0.124, 0.139, 0.185, 11000, 0.227, 1.35,  103.0, 3.17, 0.263, 0.340, 0.0645 },
        {  300, 0.0601, 0.0786, 0.0779, -1,    0.120, 0.135, 0.181,  9800, 0.247, 1.48,  112.0, 3.09, 0.264, 0.325, 0.0612 },
        {  400, 0.0470, 0.0625, 0.0616, -1,    0.115, 0.130, 0.176,  8900, 0.272, 1.62,  123.0, 3.00, 0.263, 0.312, 0.0564 },
        {  500, 0.0366, 0.0499, 0.0487, -1,    0.111, 0.126, 0.172,  8100, 0.297, 1.77,  135.0, 2.93, 0.263, 0.302, 0.0531 },
        {  630, 0.0283, 0.0403, 0.0387, -1,    0.108, 0.123, 0.169,  7300, 0.329, 1.96,  149.0, 2.86, 0.263, 0.294, 0.0504 },
        {  800, 0.0221, 0.0336, 0.0315, -1,    0.102, 0.117, 0.163,  6300, 0.381, 2.27,  173.0, 2.78, 0.263, 0.289, 0.0452 },
        { 1000, 0.0182, 0.0245, 0.0240, -1,    0.100, 0.115, 0.161,  5600, 0.427, 2.55,  194.0, 2.72, 0.263, 0.282, 0.0441 },
        { 1200, 0.0150, 0.0207, 0.0201, -1,    0.0984,0.114, 0.159,  5200, 0.461, 2.75,  209.0, 2.68, 0.263, 0.279, 0.0426 },
    };
    return db;
}

inline const CableRecord* findBySize(int sizeMm2)
{
    for (const auto& r : cableDatabase())
        if (r.sizeMm2 == sizeMm2) return &r;
    return nullptr;
}
