#include "DatabaseManager.h"
#include "CableData.h"

// Pull in the amalgamation. Because sqlite3.c is compiled as a separate CMake
// target (see CMakeLists.txt) we only need the header here.
#include <sqlite3.h>

#include <cstring>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static std::string sqliteError(sqlite3* db)
{
    return db ? sqlite3_errmsg(db) : "null handle";
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────
DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open(const std::string& path)
{
    if (m_db) close();

    int rc = sqlite3_open(path.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        m_error = sqliteError(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // Pragmas
    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA foreign_keys=ON");

    if (!createSchema())  return false;
    if (!seedIfEmpty())   return false;
    return true;
}

void DatabaseManager::close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool DatabaseManager::exec(const char* sql)
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_error = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Schema
// ─────────────────────────────────────────────────────────────────────────────
bool DatabaseManager::createSchema()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS cable_size (
            id       INTEGER PRIMARY KEY AUTOINCREMENT,
            size_mm2 INTEGER NOT NULL UNIQUE
        );

        CREATE TABLE IF NOT EXISTS cable_electrical_data (
            id                                                    INTEGER PRIMARY KEY AUTOINCREMENT,
            cable_size_id                                         INTEGER NOT NULL,
            max_dc_resistance_20C_ohm_per_km                     REAL,
            ac_resistance_50hz_90C_trefoil_touching_ohm_per_km   REAL,
            ac_resistance_50hz_90C_flat_touching_ohm_per_km      REAL,
            ac_resistance_50hz_90C_flat_spaced_ohm_per_km        REAL,
            inductive_reactance_50hz_90C_trefoil_touching_ohm_per_km REAL,
            inductive_reactance_50hz_90C_flat_touching_ohm_per_km    REAL,
            inductive_reactance_50hz_90C_flat_spaced_ohm_per_km      REAL,
            insulation_resistance_20C_Mohm_km                    REAL,
            conductor_to_screen_capacitance_uF_per_km            REAL,
            charging_current_per_phase_A_per_km                  REAL,
            dielectric_loss_per_phase_W_per_km                   REAL,
            max_dielectric_stress_kV_per_mm                      REAL,
            screen_dc_resistance_20C_ohm_per_km                  REAL,
            zero_sequence_resistance_20C_ohm_per_km              REAL,
            zero_sequence_reactance_50hz_ohm_per_km              REAL,
            FOREIGN KEY (cable_size_id) REFERENCES cable_size(id) ON DELETE CASCADE
        );
    )";

    if (!exec(sql)) {
        m_error = "Schema creation failed: " + m_error;
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Seeding
// ─────────────────────────────────────────────────────────────────────────────
bool DatabaseManager::seedIfEmpty()
{
    // Check row count
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db, "SELECT COUNT(*) FROM cable_size", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count > 0) return true;  // already seeded

    exec("BEGIN TRANSACTION");

    sqlite3_stmt* insSize  = nullptr;
    sqlite3_stmt* insElec  = nullptr;

    sqlite3_prepare_v2(m_db,
        "INSERT INTO cable_size (size_mm2) VALUES (?)",
        -1, &insSize, nullptr);

    sqlite3_prepare_v2(m_db, R"(
        INSERT INTO cable_electrical_data (
            cable_size_id,
            max_dc_resistance_20C_ohm_per_km,
            ac_resistance_50hz_90C_trefoil_touching_ohm_per_km,
            ac_resistance_50hz_90C_flat_touching_ohm_per_km,
            ac_resistance_50hz_90C_flat_spaced_ohm_per_km,
            inductive_reactance_50hz_90C_trefoil_touching_ohm_per_km,
            inductive_reactance_50hz_90C_flat_touching_ohm_per_km,
            inductive_reactance_50hz_90C_flat_spaced_ohm_per_km,
            insulation_resistance_20C_Mohm_km,
            conductor_to_screen_capacitance_uF_per_km,
            charging_current_per_phase_A_per_km,
            dielectric_loss_per_phase_W_per_km,
            max_dielectric_stress_kV_per_mm,
            screen_dc_resistance_20C_ohm_per_km,
            zero_sequence_resistance_20C_ohm_per_km,
            zero_sequence_reactance_50hz_ohm_per_km
        ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )", -1, &insElec, nullptr);

    auto bindReal = [](sqlite3_stmt* s, int col, double v) {
        if (v < 0)
            sqlite3_bind_null(s, col);
        else
            sqlite3_bind_double(s, col, v);
    };

    for (const auto& c : cableDatabase()) {
        // Insert size row
        sqlite3_bind_int(insSize, 1, c.sizeMm2);
        sqlite3_step(insSize);
        sqlite3_int64 sizeId = sqlite3_last_insert_rowid(m_db);
        sqlite3_reset(insSize);

        // Insert electrical row
        sqlite3_bind_int64(insElec,  1, sizeId);
        sqlite3_bind_double(insElec, 2,  c.maxDcResistance20C);
        sqlite3_bind_double(insElec, 3,  c.acResistanceTrefoilTouching);
        sqlite3_bind_double(insElec, 4,  c.acResistanceFlatTouching);
        bindReal(insElec,            5,  c.acResistanceFlatSpaced);
        sqlite3_bind_double(insElec, 6,  c.inductiveReactanceTrefoilTouching);
        sqlite3_bind_double(insElec, 7,  c.inductiveReactanceFlatTouching);
        sqlite3_bind_double(insElec, 8,  c.inductiveReactanceFlatSpaced);
        sqlite3_bind_double(insElec, 9,  c.insulationResistance20C);
        sqlite3_bind_double(insElec, 10, c.conductorToScreenCapacitance);
        sqlite3_bind_double(insElec, 11, c.chargingCurrentPerPhase);
        sqlite3_bind_double(insElec, 12, c.dielectricLossPerPhase);
        sqlite3_bind_double(insElec, 13, c.maxDielectricStress);
        sqlite3_bind_double(insElec, 14, c.screenDcResistance20C);
        sqlite3_bind_double(insElec, 15, c.zeroSequenceResistance20C);
        sqlite3_bind_double(insElec, 16, c.zeroSequenceReactance50Hz);
        sqlite3_step(insElec);
        sqlite3_reset(insElec);
    }

    sqlite3_finalize(insSize);
    sqlite3_finalize(insElec);
    exec("COMMIT");
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────
std::vector<CableRecord> DatabaseManager::allRecords() const
{
    std::vector<CableRecord> out;
    if (!m_db) return out;

    const char* sql = R"(
        SELECT
            s.size_mm2,
            e.max_dc_resistance_20C_ohm_per_km,
            e.ac_resistance_50hz_90C_trefoil_touching_ohm_per_km,
            e.ac_resistance_50hz_90C_flat_touching_ohm_per_km,
            e.ac_resistance_50hz_90C_flat_spaced_ohm_per_km,
            e.inductive_reactance_50hz_90C_trefoil_touching_ohm_per_km,
            e.inductive_reactance_50hz_90C_flat_touching_ohm_per_km,
            e.inductive_reactance_50hz_90C_flat_spaced_ohm_per_km,
            e.insulation_resistance_20C_Mohm_km,
            e.conductor_to_screen_capacitance_uF_per_km,
            e.charging_current_per_phase_A_per_km,
            e.dielectric_loss_per_phase_W_per_km,
            e.max_dielectric_stress_kV_per_mm,
            e.screen_dc_resistance_20C_ohm_per_km,
            e.zero_sequence_resistance_20C_ohm_per_km,
            e.zero_sequence_reactance_50hz_ohm_per_km
        FROM cable_electrical_data e
        JOIN cable_size s ON s.id = e.cable_size_id
        ORDER BY s.size_mm2
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return out;

    auto colReal = [&](int col) -> double {
        if (sqlite3_column_type(stmt, col) == SQLITE_NULL) return -1.0;
        return sqlite3_column_double(stmt, col);
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CableRecord r;
        r.sizeMm2                          = sqlite3_column_int(stmt, 0);
        r.maxDcResistance20C               = colReal(1);
        r.acResistanceTrefoilTouching      = colReal(2);
        r.acResistanceFlatTouching         = colReal(3);
        r.acResistanceFlatSpaced           = colReal(4);
        r.inductiveReactanceTrefoilTouching = colReal(5);
        r.inductiveReactanceFlatTouching   = colReal(6);
        r.inductiveReactanceFlatSpaced     = colReal(7);
        r.insulationResistance20C          = colReal(8);
        r.conductorToScreenCapacitance     = colReal(9);
        r.chargingCurrentPerPhase          = colReal(10);
        r.dielectricLossPerPhase           = colReal(11);
        r.maxDielectricStress              = colReal(12);
        r.screenDcResistance20C            = colReal(13);
        r.zeroSequenceResistance20C        = colReal(14);
        r.zeroSequenceReactance50Hz        = colReal(15);
        out.push_back(r);
    }

    sqlite3_finalize(stmt);
    return out;
}

CableRecord DatabaseManager::recordBySize(int sizeMm2) const
{
    for (const auto& r : allRecords())
        if (r.sizeMm2 == sizeMm2) return r;
    return {};
}

std::vector<int> DatabaseManager::availableSizes() const
{
    std::vector<int> out;
    if (!m_db) return out;

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_db,
        "SELECT size_mm2 FROM cable_size ORDER BY size_mm2",
        -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back(sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    return out;
}
