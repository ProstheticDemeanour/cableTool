#include "CableData.h"
#include "Calculator.h"
#include "DatabaseManager.h"
#include "SheathCalc.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/color.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
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
// Arrangement helper — UI only.
// Trefoil: Sab=Sbc=Sac=pitch
// FlatTouching: Sab=Sbc=pitch, Sac=2*pitch
// FlatSpaced: Sab=Sbc=pitch, Sac=2*pitch  (pitch entered by user as custom value)
// Custom: all three entered independently
// ─────────────────────────────────────────────────────────────────────────────

enum class SvArr { Trefoil, FlatTouch, FlatSpaced, Custom };

static const char* svArrLabel(SvArr a) {
    switch (a) {
    case SvArr::Trefoil:    return "Tref ";
    case SvArr::FlatTouch:  return "FlatT";
    case SvArr::FlatSpaced: return "FlatS";
    case SvArr::Custom:     return "Cust ";
    }
    return "";
}

static SvArr svArrNext(SvArr a) {
    switch (a) {
    case SvArr::Trefoil:    return SvArr::FlatTouch;
    case SvArr::FlatTouch:  return SvArr::FlatSpaced;
    case SvArr::FlatSpaced: return SvArr::Custom;
    case SvArr::Custom:     return SvArr::Trefoil;
    }
    return SvArr::Trefoil;
}

// Derive spacings from arrangement + the pitch string the user entered.
// For Trefoil and FlatTouch, pitch is Sab (= cable centre spacing).
// For FlatSpaced and Custom the user edits Sab/Sbc/Sac directly, so this
// function is not called — the raw strings are used straight.
static std::tuple<double,double,double>
svSpacings(SvArr arr, double pitch)
{
    switch (arr) {
    case SvArr::Trefoil:    return { pitch, pitch, pitch };
    case SvArr::FlatTouch:  return { pitch, pitch, 2.0 * pitch };
    // FlatSpaced and Custom: caller uses raw Sab/Sbc/Sac fields directly
    default:                return { pitch, pitch, 2.0 * pitch };
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// makeSheathGraph
// Canvas graph with Y-axis tick labels, X-axis distance markers, baseline,
// and labelled cross-bond markers.
//
// Layout (terminal cells):
//   yLabelW  — left column for Y-axis labels (fixed width)
//   remainder — canvas fill
// ─────────────────────────────────────────────────────────────────────────────
static Element makeSheathGraph(const sheath::SheathResults& res,
                                int graphWidth, int graphHeight)
{
    if (!res.valid || res.totalLength == 0)
        return vbox({
            filler(),
            text("  No results — press [Calculate]") | dim | center,
            filler(),
        });

    // ── Axis setup ────────────────────────────────────────────────────────────
    const int yLabelW = 7;   // chars reserved for Y-axis labels ("999 V |")

    // Canvas pixel dimensions (braille: 2px per cell col, 4px per cell row)
    const int CW = (graphWidth - yLabelW) * 2;
    const int CH = graphHeight * 4;

    const double yMax = std::max({ res.maxVoltage_A,
                                   res.maxVoltage_B,
                                   res.maxVoltage_C, 1.0 });

    // Round yMax up to a neat tick interval so labels are clean numbers
    auto niceStep = [](double range, int ticks) -> double {
        double raw  = range / ticks;
        double mag  = std::pow(10.0, std::floor(std::log10(raw)));
        double norm = raw / mag;
        double nice = (norm < 1.5) ? 1.0 : (norm < 3.5) ? 2.0 :
                      (norm < 7.5) ? 5.0 : 10.0;
        return nice * mag;
    };
    const int    nTicks  = 4;
    const double tickStep = niceStep(yMax, nTicks);
    const double yTop    = tickStep * std::ceil(yMax / tickStep);

    auto px = [&](int    m) -> int { return static_cast<int>(
        static_cast<double>(m) / res.totalLength * (CW - 1)); };
    auto py = [&](double v) -> int { return static_cast<int>(
        (1.0 - v / yTop) * (CH - 1)); };

    auto c = Canvas(CW, CH);

    // ── Baseline (y = 0) ──────────────────────────────────────────────────────
    {
        int y0 = py(0.0);
        for (int x = 0; x < CW; ++x)
            c.DrawPoint(x, y0, true, Color::GrayDark);
    }

    // ── Horizontal grid lines at each tick ───────────────────────────────────
    for (int t = 1; t <= nTicks; ++t) {
        double v = tickStep * t;
        if (v > yTop * 1.01) break;
        int y = py(v);
        for (int x = 0; x < CW; x += 4)   // dashed
            c.DrawPoint(x, y, true, Color::GrayDark);
    }

    // ── Cross-bond / minor boundary markers ──────────────────────────────────
    for (int mb : res.minorBoundaries) {
        int x = px(mb);
        // Dashed vertical — every other braille row
        for (int y = 0; y < CH; y += 2)
            c.DrawPoint(x, y, true, Color::Yellow);
        // Small "X" cap at top
        if (x > 0)   c.DrawPoint(x - 1, 0, true, Color::Yellow);
        if (x < CW-1) c.DrawPoint(x + 1, 0, true, Color::Yellow);
    }

    // ── Phase curves ─────────────────────────────────────────────────────────
    // Unrolled to avoid MSVC C2676 on std::array subscript inside lambda.
    for (int m = 1; m < res.totalLength; ++m) {
        double v0a = std::get<0>(res.Emag[m - 1]);
        double v0b = std::get<1>(res.Emag[m - 1]);
        double v0c = std::get<2>(res.Emag[m - 1]);
        double v1a = std::get<0>(res.Emag[m]);
        double v1b = std::get<1>(res.Emag[m]);
        double v1c = std::get<2>(res.Emag[m]);
        c.DrawPointLine(px(m-1), py(v0a), px(m), py(v1a), Color::Cyan);
        c.DrawPointLine(px(m-1), py(v0b), px(m), py(v1b), Color::Yellow);
        c.DrawPointLine(px(m-1), py(v0c), px(m), py(v1c), Color::Magenta);
    }

    // ── Y-axis label column ───────────────────────────────────────────────────
    // One label per tick, right-aligned into yLabelW chars, placed at the
    // correct row using vbox + filler weighting.
    // We build from top (yTop) down to 0.
    Elements yAxis;
    // Top label
    {
        std::ostringstream s;
        s << std::fixed << std::setprecision(0) << yTop << "V";
        std::string lbl = s.str();
        while ((int)lbl.size() < yLabelW - 1) lbl = " " + lbl;
        yAxis.push_back(text(lbl + "|") | color(Color::GrayDark));
    }
    // Intermediate ticks (descending)
    for (int t = nTicks - 1; t >= 1; --t) {
        double v = tickStep * t;
        // filler proportional to the gap between this tick and the one above
        yAxis.push_back(filler());
        std::ostringstream s;
        s << std::fixed << std::setprecision(0) << v << "V";
        std::string lbl = s.str();
        while ((int)lbl.size() < yLabelW - 1) lbl = " " + lbl;
        yAxis.push_back(text(lbl + "|") | color(Color::GrayDark));
    }
    // Bottom (0)
    yAxis.push_back(filler());
    yAxis.push_back(text("   0V|") | color(Color::GrayDark));

    Element yAxisEl = vbox(yAxis) | size(WIDTH, EQUAL, yLabelW)
                                  | size(HEIGHT, EQUAL, graphHeight);

    // ── X-axis distance labels ────────────────────────────────────────────────
    // Show ~5 distance markers along the bottom.
    const int xTicks = 5;
    Elements xLabels;
    xLabels.push_back(text(std::string(yLabelW, ' ')));  // blank under Y axis
    int lastLabelEnd = 0;
    for (int t = 0; t <= xTicks; ++t) {
        int m = res.totalLength * t / xTicks;
        int cellX = yLabelW + (m * (graphWidth - yLabelW)) / res.totalLength;

        std::string lbl = std::to_string(m) + "m";
        int lblW = static_cast<int>(lbl.size());

        // Pad to reach cellX from end of last label, avoid overlap
        int pad = cellX - lastLabelEnd - lblW / 2;
        if (pad > 0) xLabels.push_back(text(std::string(pad, ' ')));
        xLabels.push_back(text(lbl) | color(Color::GrayDark));
        lastLabelEnd = cellX + (lblW + 1) / 2;
    }

    // ── Assemble ──────────────────────────────────────────────────────────────
    return vbox({
        hbox({ yAxisEl, canvas(std::move(c)) | flex }),
        hbox(xLabels),
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// makeSheathTab
// ─────────────────────────────────────────────────────────────────────────────
static Element makeSheathTab(Element leftPanel,
                              const sheath::SheathResults& results)
{
    Element right;
    if (!results.valid) {
        right = window(
            text(" Sheath Voltage Profile "),
            vbox({ filler(),
                   text("  Define route sections and press [Calculate]")
                       | dim | center,
                   filler() }) | flex
        ) | flex;
    } else {
        auto srow = [](const std::string& lbl, const std::string& val,
                       const std::string& unit, Color col) {
            return hbox({
                text("  " + lbl) | dim, filler(),
                text(val) | bold | color(col),
                text(" " + unit) | dim, text("  "),
            });
        };
        auto fv = [](double v) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(1) << v;
            return s.str();
        };
        right = window(
            text(" Sheath Voltage Profile "),
            vbox({
                makeSheathGraph(results, 80, 16),
                separator(),
                hbox({
                    text("  ── ") | color(Color::Cyan),
                    text("Phase A   ") | dim,
                    text("── ") | color(Color::Yellow),
                    text("Phase B   ") | dim,
                    text("── ") | color(Color::Magenta),
                    text("Phase C   ") | dim,
                    text("-- ") | color(Color::Yellow),
                    text("Cross-bond") | dim,
                }),
                separator(),
                srow("Peak sheath voltage  A", fv(results.maxVoltage_A), "V", Color::Cyan),
                srow("Peak sheath voltage  B", fv(results.maxVoltage_B), "V", Color::Yellow),
                srow("Peak sheath voltage  C", fv(results.maxVoltage_C), "V", Color::Magenta),
                separator(),
                srow("Total route length",
                     std::to_string(results.totalLength) + " m", "", Color::White),
                srow("Cross-bond points",
                     std::to_string(results.minorBoundaries.size()), "", Color::White),
            }) | flex
        ) | flex;
    }
    return hbox({ leftPanel | size(WIDTH, EQUAL, 72), right }) | flex;
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
    std::vector<std::string> tabLabels = {
        "  System  ", "  Sheath Voltage  ", "  Cable Data  "
    };
    int activeTab = 0;

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
    // focused_entry keeps the selected item scrolled into view automatically
    auto sizeMenuOpt        = MenuOption::Vertical();
    sizeMenuOpt.focused_entry = &sizeIdx;
    auto sizeMenu           = Menu(&sizeLabels, &sizeIdx, sizeMenuOpt);

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
                sizeMenu->Render() | frame
                                   | size(HEIGHT, LESS_THAN, 8),
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

    // ── Sheath Voltage tab ───────────────────────────────────────────────────
    //
    // Architecture notes (fixes for crash + lock-up):
    //
    // 1. Rows stored in std::deque<SvRow> — deque never moves existing elements
    //    on push_back, so &row.length etc. remain valid forever.
    //
    // 2. Components created ONCE per row in a parallel std::deque<SvRowComps>.
    //    We never call rebuildSvRowComps or buildSvContainer — the container
    //    is built once and rows are added/removed by adding/hiding components.
    //
    // 3. svRenderer wraps a single stable Container::Vertical.  Adding a row
    //    calls container->Add(); removing calls DetachAllChildren + re-Add.
    //
    // 4. svCurrStr starts as "0" not "" so stod never throws on first calc.

    struct SvRow {
        SvArr       arr       = SvArr::Trefoil;
        bool        transpose = false;
        std::string length    = "10";
        std::string Sab       = "160";
        std::string Sbc       = "160";
        std::string Sac       = "160";
        std::string label     = "";
    };

    // Stable storage — deque pointers survive push_back
    std::deque<SvRow> svRows;
    svRows.push_back(SvRow{});

    sheath::SheathResults svResults;
    std::string           svError;
    int                   svSelectedRow = 0;

    std::string svCurrStr    = "0";
    std::string svFreqStr    = "50";
    int         svFormulaIdx = 1;
    std::vector<std::string> svFormulaLabels = { "Simplified", "Full" };

    // Fixed top-level inputs
    auto svCurrInput    = Input(&svCurrStr, "A");
    auto svFreqInput    = Input(&svFreqStr, "Hz");
    auto svFormulaRadio = Radiobox(&svFormulaLabels, &svFormulaIdx);

    // Per-row component bundle — created once, stored stably
    struct SvRowComps {
        Component length, Sab, Sbc, Sac, label;
    };
    std::deque<SvRowComps> svRowComps;

    // Create components for one row, binding to its stable string addresses
    auto makeRowComps = [](SvRow& row) -> SvRowComps {
        return {
            Input(&row.length, "m"),
            Input(&row.Sab,    "mm"),
            Input(&row.Sbc,    "mm"),
            Input(&row.Sac,    "mm"),
            Input(&row.label,  "label"),
        };
    };
    svRowComps.push_back(makeRowComps(svRows[0]));

    // The rows sub-container — we Add/DetachAllChildren on this directly
    auto svRowsContainer = Container::Vertical({
        svRowComps[0].length,
        svRowComps[0].Sab,
        svRowComps[0].Sbc,
        svRowComps[0].Sac,
        svRowComps[0].label,
    });

    // Rebuild the rows sub-container from svRowComps (called after add/del)
    auto refreshRowsContainer = [&]() {
        svRowsContainer->DetachAllChildren();
        for (auto& rc : svRowComps) {
            svRowsContainer->Add(rc.length);
            svRowsContainer->Add(rc.Sab);
            svRowsContainer->Add(rc.Sbc);
            svRowsContainer->Add(rc.Sac);
            svRowsContainer->Add(rc.label);
        }
    };

    auto svCalcButton = Button("  Calculate  ", [&] {
        svError.clear();
        try {
            sheath::SheathParams p;
            p.current_A    = std::stod(svCurrStr);
            p.frequency_Hz = std::stod(svFreqStr);
            if (p.current_A <= 0.0) {
                svError = "Current must be > 0";
                return;
            }
            p.formula = (svFormulaIdx == 0)
                        ? sheath::SheathParams::Formula::SIMPLIFIED
                        : sheath::SheathParams::Formula::FULL;

            for (const auto& row : svRows) {
                sheath::RouteSection sec;
                sec.length_m  = std::stod(row.length);
                sec.transpose = row.transpose;
                sec.label     = row.label;

                if (row.arr == SvArr::Trefoil || row.arr == SvArr::FlatTouch) {
                    auto [sab, sbc, sac] = svSpacings(row.arr, std::stod(row.Sab));
                    sec.Sab_mm = sab;
                    sec.Sbc_mm = sbc;
                    sec.Sac_mm = sac;
                } else {
                    sec.Sab_mm = std::stod(row.Sab);
                    sec.Sbc_mm = std::stod(row.Sbc);
                    sec.Sac_mm = std::stod(row.Sac);
                }
                p.route.push_back(sec);
            }
            svResults = sheath::calculate(p);
            if (!svResults.valid) svError = svResults.errorMsg;
        } catch (const std::exception& ex) {
            svError = std::string("Parse error: ") + ex.what();
        } catch (...) {
            svError = "Parse error — check numeric fields";
        }
    }, ButtonOption::Animated(Color::Green));

    auto svAddRowButton = Button(" + ", [&] {
        svRows.push_back(SvRow{});
        svRowComps.push_back(makeRowComps(svRows.back()));
        refreshRowsContainer();
    }, ButtonOption::Simple());

    auto svDelRowButton = Button(" - ", [&] {
        if (svRows.size() > 1) {
            svRows.pop_back();
            svRowComps.pop_back();
            svSelectedRow = std::min(svSelectedRow,
                                     static_cast<int>(svRows.size()) - 1);
            refreshRowsContainer();
        }
    }, ButtonOption::Simple());

    auto svClearButton = Button(" Clear ", [&] {
        svRows.clear();
        svRowComps.clear();
        svRows.push_back(SvRow{});
        svRowComps.push_back(makeRowComps(svRows[0]));
        svResults     = {};
        svSelectedRow = 0;
        svError.clear();
        refreshRowsContainer();
    }, ButtonOption::Simple());

    // Single stable container — never rebuilt
    auto svContainer = Container::Vertical({
        svCurrInput, svFreqInput, svFormulaRadio,
        svRowsContainer,
        svAddRowButton, svDelRowButton, svClearButton,
        svCalcButton,
    });

    auto svRenderer = Renderer(svContainer, [&]() -> Element {

        auto li = [](const std::string& lbl, Element inp) {
            return hbox({
                text(lbl) | dim | size(WIDTH, EQUAL, 20),
                inp       | size(WIDTH, EQUAL, 12),
            });
        };
        Element sysPane = window(
            text(" System Parameters "),
            vbox({
                li("Design current (A) : ", svCurrInput->Render()),
                li("Frequency    (Hz) : ", svFreqInput->Render()),
                separator(),
                text(" EMF formula:") | dim,
                hbox({ text("   "), svFormulaRadio->Render() }),
            })
        );

        // Compute auto-derived spacings for display in Sbc/Sac columns
        // when arrangement is Trefoil or FlatTouch.
        auto derivedSbc = [&](const SvRow& row) -> std::string {
            if (row.arr == SvArr::Trefoil || row.arr == SvArr::FlatTouch) {
                try {
                    auto [sab, sbc, sac] = svSpacings(row.arr, std::stod(row.Sab));
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(0) << sbc;
                    return s.str();
                } catch (...) { return "?"; }
            }
            return "";
        };
        auto derivedSac = [&](const SvRow& row) -> std::string {
            if (row.arr == SvArr::Trefoil || row.arr == SvArr::FlatTouch) {
                try {
                    auto [sab, sbc, sac] = svSpacings(row.arr, std::stod(row.Sab));
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(0) << sac;
                    return s.str();
                } catch (...) { return "?"; }
            }
            return "";
        };

        // Column widths:  sel(1) #(3) arr(7) xb(4) len(8) sab(9) sbc(9) sac(9) label(flex)
        Elements rows;
        {
            Elements hdr;
            hdr.push_back(text(" ")          | size(WIDTH, EQUAL, 1));
            hdr.push_back(text("#")          | bold | size(WIDTH, EQUAL, 3));
            hdr.push_back(text("Arr   ")     | bold | size(WIDTH, EQUAL, 7));
            hdr.push_back(text("XB  ")       | bold | size(WIDTH, EQUAL, 4));
            hdr.push_back(text("Len(m)  ")   | bold | size(WIDTH, EQUAL, 8));
            hdr.push_back(text("Sab(mm)  ")  | bold | size(WIDTH, EQUAL, 9));
            hdr.push_back(text("Sbc(mm)  ")  | bold | size(WIDTH, EQUAL, 9));
            hdr.push_back(text("Sac(mm)  ")  | bold | size(WIDTH, EQUAL, 9));
            hdr.push_back(text("Label")      | bold | flex);
            rows.push_back(hbox(hdr) | bgcolor(Color::Blue));
        }

        for (size_t i = 0; i < svRows.size(); ++i) {
            const auto& row = svRows[i];
            const auto& rc  = svRowComps[i];
            bool sel      = (static_cast<int>(i) == svSelectedRow);
            bool autoCalc = (row.arr == SvArr::Trefoil || row.arr == SvArr::FlatTouch);

            Color arrCol = Color::White;
            switch (row.arr) {
            case SvArr::Trefoil:    arrCol = Color::Cyan;    break;
            case SvArr::FlatTouch:  arrCol = Color::Green;   break;
            case SvArr::FlatSpaced: arrCol = Color::Yellow;  break;
            case SvArr::Custom:     arrCol = Color::Magenta; break;
            }

            Elements cells;
            cells.push_back(
                text(sel ? ">" : " ") | color(Color::Yellow) | size(WIDTH, EQUAL, 1));
            cells.push_back(
                text(std::to_string(i + 1)) | size(WIDTH, EQUAL, 3));
            cells.push_back(
                text(svArrLabel(row.arr)) | color(arrCol) | size(WIDTH, EQUAL, 7));
            cells.push_back(
                text(row.transpose ? "[X] " : "[ ] ")
                | size(WIDTH, EQUAL, 4)
                | (row.transpose ? color(Color::Green) : color(Color::GrayDark)));
            cells.push_back(rc.length->Render() | size(WIDTH, EQUAL, 8));
            cells.push_back(rc.Sab->Render()    | size(WIDTH, EQUAL, 9));
            // Sbc — editable for FlatSpaced/Custom, computed+dim for others
            cells.push_back(autoCalc
                ? text(derivedSbc(row)) | dim | size(WIDTH, EQUAL, 9)
                : rc.Sbc->Render()      | size(WIDTH, EQUAL, 9));
            // Sac — editable for FlatSpaced/Custom, computed+dim for others
            cells.push_back(autoCalc
                ? text(derivedSac(row)) | dim | size(WIDTH, EQUAL, 9)
                : rc.Sac->Render()      | size(WIDTH, EQUAL, 9));
            cells.push_back(rc.label->Render() | flex);

            auto rowBg = sel ? bgcolor(Color::GrayDark) : nothing;
            rows.push_back(hbox(cells) | rowBg);
        }

        Element routePane = window(
            text(" Route Sections   "
                 "[ \u2191\u2193 select | A arrangement | T cross-bond ]"),
            vbox({
                vbox(rows),
                separator(),
                hbox({
                    svAddRowButton->Render(),
                    text(" "),
                    svDelRowButton->Render(),
                    text(" "),
                    svClearButton->Render(),
                }),
                svError.empty()
                    ? text(" ")
                    : text(" [!] " + svError) | color(Color::Red),
            }) | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 26)
        );

        return makeSheathTab(
            vbox({ sysPane, routePane, svCalcButton->Render() | center }),
            svResults
        );
    });

    // ── Cable Data tab ────────────────────────────────────────────────────────
    auto cableDataContainer = Container::Vertical({});

    auto cableDataComp = Renderer(cableDataContainer, [&] {
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
    auto tabContent    = Container::Tab(
        { systemRenderer, svRenderer, cableDataComp }, &activeTab);
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
                text(" CableDesign v1.2.0 ") | dim,
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
        if (activeTab == 1) {
            // ↑ / ↓ — move row selection highlight
            if (event == Event::ArrowUp) {
                if (svSelectedRow > 0) --svSelectedRow;
                return false;   // also advance FTXUI focus
            }
            if (event == Event::ArrowDown) {
                if (svSelectedRow < static_cast<int>(svRows.size()) - 1)
                    ++svSelectedRow;
                return false;
            }
            // A — cycle arrangement on highlighted row
            if (event == Event::Character('a') || event == Event::Character('A')) {
                if (!svRows.empty()) {
                    svRows[svSelectedRow].arr =
                        svArrNext(svRows[svSelectedRow].arr);
                }
                return true;
            }
            // T — toggle cross-bond on highlighted row
            if (event == Event::Character('t') || event == Event::Character('T')) {
                if (!svRows.empty())
                    svRows[svSelectedRow].transpose = !svRows[svSelectedRow].transpose;
                return true;
            }
            // + / - — add / remove row without reaching for the mouse
            if (event == Event::Character('+')) {
                svAddRowButton->OnEvent(Event::Return);
                return true;
            }
            if (event == Event::Character('-')) {
                svDelRowButton->OnEvent(Event::Return);
                return true;
            }
        }
        return false;
    });

    screen.Loop(appWithKeys);
    return 0;
}
