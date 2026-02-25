#include "CableData.h"
#include "Calculator.h"
#include "DatabaseManager.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace ftxui;

// ── Formatting helpers ────────────────────────────────────────────────────────
static std::string fmt(double v, int dp = 4) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(dp) << v;
    return ss.str();
}
static std::string fmtOpt(double v, int dp = 4) {
    return (v < 0) ? "  -  " : fmt(v, dp);
}

// ── Cable Data table element ──────────────────────────────────────────────────
static Element makeCableTable(const std::vector<CableRecord>& records)
{
    std::vector<std::string> hdr1 = {
        "Size",  "DC Res", "AC Res",  "AC Res",  "AC Res",
        "X",     "X",      "X Flat",  "Ins Res", "Cap",
        "Ic",    "Diel",   "Stress",  "Scr Res", "Z0 R",  "Z0 X"
    };
    std::vector<std::string> hdr2 = {
        "mm2",  "20C",    "Trefoil", "FlatTch", "FlatSpc",
        "Trefoil","FlatTch","Spaced", "MOhm-km", "uF/km",
        "A/km", "W/km",   "kV/mm",  "Ohm/km",  "Ohm/km", "Ohm/km"
    };

    std::vector<Elements> rows;

    {
        Elements row;
        for (auto& s : hdr1) row.push_back(text(s) | bold | center);
        rows.push_back(row);
    }
    {
        Elements row;
        for (auto& s : hdr2) row.push_back(text(s) | dim | center);
        rows.push_back(row);
    }

    bool alt = false;
    for (const auto& c : records) {
        Elements row;
        auto cell = [&](const std::string& s) -> Element {
            Element e = text(s) | align_right;
            if (alt) e = e | bgcolor(Color::GrayDark);
            return e;
        };
        row.push_back(cell(std::to_string(c.sizeMm2)));
        row.push_back(cell(fmt(c.maxDcResistance20C)));
        row.push_back(cell(fmt(c.acResistanceTrefoilTouching)));
        row.push_back(cell(fmt(c.acResistanceFlatTouching)));
        row.push_back(cell(fmtOpt(c.acResistanceFlatSpaced)));
        row.push_back(cell(fmt(c.inductiveReactanceTrefoilTouching)));
        row.push_back(cell(fmt(c.inductiveReactanceFlatTouching)));
        row.push_back(cell(fmt(c.inductiveReactanceFlatSpaced)));
        row.push_back(cell(fmt(c.insulationResistance20C, 0)));
        row.push_back(cell(fmt(c.conductorToScreenCapacitance, 3)));
        row.push_back(cell(fmt(c.chargingCurrentPerPhase, 3)));
        row.push_back(cell(fmt(c.dielectricLossPerPhase, 1)));
        row.push_back(cell(fmt(c.maxDielectricStress, 2)));
        row.push_back(cell(fmt(c.screenDcResistance20C)));
        row.push_back(cell(fmt(c.zeroSequenceResistance20C)));
        row.push_back(cell(fmt(c.zeroSequenceReactance50Hz)));
        rows.push_back(row);
        alt = !alt;
    }

    auto table = Table(rows);
    for (int col = 0; col < 16; ++col) {
    table.SelectColumn(col).DecorateCells(
        size(WIDTH, GREATER_THAN, 10)
        );
    }
    table.SelectAll().Border(LIGHT);
    table.SelectRow(0).Border(HEAVY);
    table.SelectRow(0).Decorate(bold);
    table.SelectRow(1).Decorate(dim);
    table.SelectColumns(0, 0).DecorateCells(bold);

    return table.Render() | vscroll_indicator | frame | flex;
}

// ── Output panel ──────────────────────────────────────────────────────────────
static Element makeOutputPanel(const CalcResults& r, const SystemParams& p,
                                bool calculated)
{
    if (!calculated) {
        return window(
            text(" Calculated Outputs "),
            text("  Press [Enter] or [F5] to calculate...") | dim | center | flex
        ) | flex;
    }

    const std::string arrName =
        (p.arrangement == Arrangement::TrefoilTouching) ? "Trefoil Touching" :
        (p.arrangement == Arrangement::FlatTouching)    ? "Flat Touching"    :
                                                          "Flat Spaced";

    auto row = [](const std::string& lbl, const std::string& val,
                  const std::string& unit = "") {
        return hbox({
            text("  " + lbl) | dim,
            filler(),
            text(val) | bold,
            text(" " + unit) | dim,
            text("  "),
        });
    };

    auto section = [](const std::string& title) {
        return hbox({ text("--- " + title + " ") | color(Color::Cyan), filler() });
    };

    return window(
        text(" Calculated Outputs "),
        vbox({
            hbox({ text("  "), text(std::to_string(p.sizeMm2) + " mm2  -  " + arrName) | bold }),
            hbox({ text("  Length: ") | dim, text(fmt(p.lengthKm, 3) + " km") | bold }),
            separator(),
            section("Impedance"),
            row("R  (total)",  fmt(r.R),  "Ohm"),
            row("X  (total)",  fmt(r.X),  "Ohm"),
            row("Z  (total)",  fmt(r.Z),  "Ohm"),
            separator(),
            section("Load"),
            row("Apparent power",    fmt(p.powerMVA, 3),  "MVA"),
            row("Active power",      fmt(r.P_MW,     3),  "MW"),
            row("Reactive power",    fmt(r.Q_Mvar,   3),  "Mvar"),
            row("Full-load current", fmt(r.current,  1),  "A"),
            separator(),
            section("Voltage Drop"),
            row("dV (L-L)",  fmt(r.deltaV_V,   1), "V"),
            row("dV",        fmt(r.deltaV_pct, 2), "%"),
            separator(),
            section("Losses"),
            row("Resistive",  fmt(r.losses_kW,   2), "kW"),
            row("Dielectric", fmt(r.dielLoss_kW, 2), "kW  (3-phase)"),
            row("Total Power Loss", fmt(r.losses_pct, 2), "%"),
            separator(),
            section("Capacitive"),
            row("Charging current", fmt(r.chargingA, 3), "A/phase"),
            separator(),
            text("  NOTE: indicative results only.") | dim,
        }) | flex
    ) | flex;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main()
{
    // ── Open database ─────────────────────────────────────────────────────────
    DatabaseManager db;
    std::string dbError;

    if (!db.open()) {
        dbError = "DB error: " + db.errorMessage();
    }

    // Load records once at startup
    std::vector<CableRecord> allRecords = db.isOpen()
        ? db.allRecords()
        : std::vector<CableRecord>{};
    std::vector<int> sizes = db.isOpen()
        ? db.availableSizes()
        : std::vector<int>{};

    // Graceful fallback to static data if DB failed
    if (allRecords.empty()) {
        allRecords = std::vector<CableRecord>(
            cableDatabase().begin(), cableDatabase().end());
        for (const auto& r : allRecords) sizes.push_back(r.sizeMm2);
    }

    // ── Tab state ─────────────────────────────────────────────────────────────
    int activeTab = 0;
    std::vector<std::string> tabLabels = { "  System  ", "  Cable Data  " };

    // ── System tab state ──────────────────────────────────────────────────────
    std::string voltageStr     = "33.0";
    std::string powerStr       = "10.0";
    std::string pfStr          = "0.95";
    std::string lengthStr      = "1.0";
    int         arrangementIdx = 0;
    int         sizeIdx        = 0;

    for (int i = 0; i < (int)sizes.size(); ++i)
        if (sizes[i] == 240) { sizeIdx = i; break; }

    std::vector<std::string> arrangementLabels = {
        "Trefoil Touching", "Flat Touching", "Flat Spaced"
    };
    std::vector<std::string> sizeLabels;
    for (int s : sizes)
        sizeLabels.push_back(std::to_string(s) + " mm2");

    CalcResults results;
    bool        calculated = false;
    std::string errorMsg   = dbError;

    // ── Screen + components ───────────────────────────────────────────────────
    auto screen          = ScreenInteractive::Fullscreen();
    auto voltageInput    = Input(&voltageStr,  "33.0");
    auto powerInput      = Input(&powerStr,    "10.0");
    auto pfInput         = Input(&pfStr,       "0.95");
    auto lengthInput     = Input(&lengthStr,   "1.0");
    auto arrangementMenu = Radiobox(&arrangementLabels, &arrangementIdx);
    auto sizeMenu        = Menu(&sizeLabels, &sizeIdx);

    auto calcButton = Button("  Calculate [Enter]  ", [&] {
        errorMsg.clear();
        try {
            SystemParams p;
            p.voltageKV   = std::stod(voltageStr);
            p.powerMVA    = std::stod(powerStr);
            p.powerFactor = std::stod(pfStr);
            p.lengthKm    = std::stod(lengthStr);
            p.arrangement = static_cast<Arrangement>(arrangementIdx);
            p.sizeMm2     = sizes[sizeIdx];

            if (p.voltageKV <= 0 || p.powerMVA <= 0 ||
                p.powerFactor <= 0 || p.powerFactor > 1 || p.lengthKm <= 0) {
                errorMsg = "Invalid input - check values are positive and PF <= 1";
                return;
            }

            CableRecord cable = db.isOpen()
                ? db.recordBySize(p.sizeMm2)
                : [&]() -> CableRecord {
                    const CableRecord* p2 = findBySize(p.sizeMm2);
                    return p2 ? *p2 : CableRecord{};
                  }();

            results    = calculate(p, cable);
            calculated = true;
        } catch (...) {
            errorMsg = "Parse error - ensure all fields contain valid numbers";
        }
    }, ButtonOption::Animated(Color::Green));

    // ── System tab ────────────────────────────────────────────────────────────
    auto systemInputs = Container::Vertical({
        voltageInput, powerInput, pfInput, lengthInput,
        arrangementMenu, sizeMenu, calcButton
    });

    auto systemRenderer = Renderer(systemInputs, [&] {
        SystemParams p;
        try {
            p.voltageKV   = std::stod(voltageStr);
            p.powerMVA    = std::stod(powerStr);
            p.powerFactor = std::stod(pfStr);
            p.lengthKm    = std::stod(lengthStr);
        } catch (...) {}
        if (!sizes.empty()) p.sizeMm2 = sizes[sizeIdx];
        p.arrangement = static_cast<Arrangement>(arrangementIdx);

        auto labelledInput = [](const std::string& lbl, Element inp) {
            return hbox({
                text(lbl) | dim | size(WIDTH, EQUAL, 22),
                inp | size(WIDTH, EQUAL, 16),
            });
        };

        Element inputPane = window(
            text(" System Parameters "),
            vbox({
                labelledInput("Voltage (L-L) [kV] : ", voltageInput->Render()),
                labelledInput("Apparent Power [MVA]: ", powerInput->Render()),
                labelledInput("Power Factor        : ", pfInput->Render()),
                labelledInput("Cable Length [km]   : ", lengthInput->Render()),
                separator(),
                text(" Arrangement:") | dim,
                hbox({ text("  "), arrangementMenu->Render() }),
                separator(),
                text(" Conductor Size:") | dim,
                sizeMenu->Render() | size(HEIGHT, LESS_THAN, 8)
                                   | vscroll_indicator | frame,
                separator(),
                calcButton->Render() | center,
                errorMsg.empty()
                    ? text("")
                    : text(" [!] " + errorMsg) | color(Color::Red),
            })
        );

        return hbox({
            inputPane  | size(WIDTH, EQUAL, 46),
            makeOutputPanel(results, p, calculated) | flex,
        }) | flex;
    });

    // ── Cable Data tab ────────────────────────────────────────────────────────
    auto cableDataComp = Renderer([&] {
        return vbox({
            text(" 33 kV XLPE Cable Electrical Data") | bold | center,
            text(db.isOpen()
                ? " Source: cable_design.db"
                : " Source: built-in fallback (DB unavailable)") | dim | center,
            separator(),
            makeCableTable(allRecords),
        }) | flex;
    });

    // ── Root layout ───────────────────────────────────────────────────────────
    auto tabToggle     = Toggle(&tabLabels, &activeTab);
    auto tabContent    = Container::Tab({ systemRenderer, cableDataComp }, &activeTab);
    auto mainContainer = Container::Vertical({ tabToggle, tabContent });

    auto mainRenderer = Renderer(mainContainer, [&] {
        return vbox({
            hbox({
                text(" Cable Design Tool ") | bold | color(Color::Yellow),
                filler(),
                text(" Tab/Shift+Tab: switch tabs   q: quit ") | dim,
            }) | bgcolor(Color::Blue),
            tabToggle->Render() | center,
            separator(),
            tabContent->Render() | flex,
            hbox({
                text(" F5/Enter = Calculate  |  DB: ") | dim,
                db.isOpen()
                    ? text("cable_design.db  OK") | color(Color::Green)
                    : text("unavailable") | color(Color::Red),
                filler(),
                text(" CableDesign v0.1.2 ") | dim,
            }) | bgcolor(Color::GrayDark),
        }) | flex;
    });

    auto appWithKeys = CatchEvent(mainRenderer, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == Event::F5) {
            calcButton->OnEvent(Event::Return);
            return true;
        }
        return false;
    });

    screen.Loop(appWithKeys);
    return 0;
}
