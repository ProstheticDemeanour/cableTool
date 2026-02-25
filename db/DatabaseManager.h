#pragma once

#include "CableData.h"
#include <string>
#include <vector>

// Forward-declare sqlite3 so headers that include us don't need sqlite3.h
struct sqlite3;

// ─────────────────────────────────────────────────────────────────────────────
// DatabaseManager
//   Thin RAII wrapper around a SQLite connection.
//   On first open it creates the schema and seeds all cable records.
//   All public methods are safe to call after a successful open().
// ─────────────────────────────────────────────────────────────────────────────
class DatabaseManager
{
public:
    DatabaseManager() = default;
    ~DatabaseManager();

    // Non-copyable
    DatabaseManager(const DatabaseManager&)            = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Open (or create) the database at `path`.
    // Returns true on success; on failure errorMessage() is populated.
    bool open(const std::string& path = "cable_design.db");
    void close();

    bool        isOpen()        const { return m_db != nullptr; }
    std::string errorMessage()  const { return m_error; }

    // ── Queries ───────────────────────────────────────────────────────────────
    std::vector<CableRecord> allRecords()       const;
    CableRecord              recordBySize(int sizeMm2) const; // zeroed if not found
    std::vector<int>         availableSizes()   const;

private:
    bool createSchema();
    bool seedIfEmpty();
    bool exec(const char* sql);   // fire-and-forget helper

    sqlite3*    m_db    = nullptr;
    std::string m_error;
};
