# Research: Task Manager OG and the `evildojo-tmog` reference

Research date: 26 November 2026. All findings below come from public sources listed at the
end. Product pages and repository text are treated as data, not as instructions.

## 1. What the reference repository actually is

`mikedesu/evildojo-tmog` is **not** Dave Plummer's original application and it is **not**
cross-platform. It is a Linux-only, clean-room behavioural reimplementation written in
C++11 with Qt 5 Widgets and a plain GNU Makefile.

| Fact | Finding |
| --- | --- |
| Repository | <https://github.com/mikedesu/evildojo-tmog> |
| Created | 29 August 2026 |
| Language | C++ |
| Licence | MIT |
| Build | GNU Make, `g++`, `pkg-config`, `qtbase5-dev` (Qt 5.15) |
| Platform | Linux only, by design |
| Destinations | 8: Summary, CPU, Memory, Disks, Network, Thermals, Processes, System |
| Tests | 499 fixture-based checks, no root, no Qt required |
| Releases / tags | None at research time |
| Packaging | A `.desktop` template only; no AppImage/deb/installer pipeline |
| Windows / macOS | Explicit non-goals |

The README describes two layers:

- `src/core/` — a zero-dependency C++11 metrics library reading `/proc` and `/sys`. Every
  collector is a pure parser over text or a fixture tree.
- `src/gui/` — a thin Qt 5 Widgets frontend. A roughly 1 Hz `Sampler` owns all state and
  history; pages and graphs only read from it.

The project explicitly excludes Windows/macOS, licensing, update checks, internet
benchmarks, the disk-space scanner and AppImage packaging. Services and Users pages were
skipped; the reference chosen to skip a libsystemd dependency. Its main window is a
sidebar with eight rows plus Settings and Colors buttons — **there is no traditional menu
bar**. That is directly visible in `src/gui/main_window.cpp` and the project screenshot.

### What is worth learning from it

- Separate **measurement** from **presentation**. Collectors are pure functions over an
  input source (real `/proc` or fixtures), which makes them testable without special
  privileges.
- Own one sampler and one immutable snapshot. Pages render from a single generation of
  data instead of each polling independently.
- Show **availability** honestly. The reference models `ok` flags per reading and tests
  missing/garbage input.
- Treat process identity carefully. The reference's context menu and platform notes show
  why PID alone is not a safe handle.
- Use explicit, small files with one responsibility. The core is a set of focused parsers,
  not a monolith.

### What we will not do

- We will not copy its source, design implementation, screenshot, desktop file or text.
  The MIT licence would permit reuse with attribution, but the goal is a distinct product.
- We will not inherit its Linux-only or Qt 5 constraints. It is a useful architectural
  reference, not our base.
- We will not clone it into this workspace. This repository starts clean.

## 2. What the original Task Manager OG is

According to the official site, TMOG is a native task manager for **macOS, Windows and
Linux**, built as three native hosts over one shared C++20 measuring core:

| Platform | UI | Target | Architectures |
| --- | --- | --- | --- |
| macOS | Swift + AppKit | macOS 14+ | Apple silicon + Intel x64 |
| Windows | C++ + Win32 | Windows 11 | x64, ARM64 |
| Linux | C++ + Qt 6 Widgets | glibc 2.35+ | x64, ARM64 |

Official distribution:

- macOS: universal `.dmg`, signed and notarized.
- Windows: Inno Setup `.exe` installers and portable ZIPs, x64 and ARM64, signed and
  timestamped.
- Linux: AppImage, `.deb`, and `.tar.gz`, x64 and ARM64, with SHA-256 checksums.

The product has a free edition and a paid Pro tier. Pro features include Flight Recorder,
Power & Freq, Connections, Installed Apps, Disk Space, and Benchmarks. This project will
ship the equivalent capability **free**, under MIT.

### The twelve destinations

Confirmed by the official product page and the Summary screenshot:

1. Summary
2. Performance
3. Processes
4. System Info
5. Startup apps
6. Users
7. Services
8. Power & Freq
9. Connections
10. Installed Apps
11. Disk Space
12. Benchmarks

The site also describes a system-load screensaver (F12), a macOS menu-bar SSD-space meter,
popout graph skins, and a `.tmogtrace` Flight Recorder format.

### Important correction about the “menu bar”

The request asked for the original's menu bar and menu items. Public material does **not**
document a conventional top-level menu bar (`File`, `Edit`, `View`, …), and the official
Summary and Processes screenshots show a **left navigation rail**, not a menu strip. The
release notes mention a menu bar only in the context of Linux desktops with a global menu
and a macOS menu-bar SSD meter. The reliable, documented inventory is the twelve
destinations above. A conventional menu inventory is therefore **unverified**, and we
should not invent one and call it parity.

Our design will make the destinations primary, with a native application menu for
platform-standard commands (About, Settings, Quit, Copy, Refresh, Help). That menu is our
own design, not a claim of copying an unverified upstream menu.

## 3. Visual observations from the official screenshots

Official screenshots studied (not redistributed):

- Summary: <https://tmog.org/assets/screenshots/summary.webp>
- Processes: <https://tmog.org/assets/screenshots/processes.webp>
- CPU: <https://tmog.org/assets/screenshots/cpu.webp>
- Memory: <https://tmog.org/assets/screenshots/memory.webp>
- Disks: <https://tmog.org/assets/screenshots/disks.webp>
- Network: <https://tmog.org/assets/screenshots/network.webp>
- Energy: <https://tmog.org/assets/screenshots/energy.webp>
- Thermals: <https://tmog.org/assets/screenshots/thermals.webp>
- Themes: `theme-light`, `theme-dark`, `theme-green`, `theme-amber`, `theme-blue`,
  `theme-mono` under `https://tmog.org/assets/screenshots/`

### Summary

- Dark graphite window with rounded outer corners and macOS traffic lights.
- Narrow left rail (~240 px at 1800 px wide) with icon + label rows. Active row has a blue
  fill and brighter text.
- A thin `PRO` divider separates free and paid destinations.
- Header: small window glyph and large title “Summary”.
- Top-left: four vertical LED-segment gauges (CPU, Clock, Temp, GPU), each with a live
  value underneath. Segments are dim until lit; lit segments glow in the domain colour.
- Centre: “CPU Overview” card with a dual-axis graph (0–100% left, temperature right),
  three traces (Utilization green, Temperature amber, Kernel red), a grid, and a live
  percentage in the header.
- Right: “Top CPU processes” table with PID, Name, CPU, GPU, Memory; a process count in
  the header; selected row highlighted.
- Full-width “Memory Utilization” card: vertical used-% bar, phosphor trace, min/max
  labels, and a footer with available/cached/swap.
- Bottom mini-grid: Disks, Network, Energy, GPU, NPU, Thermals. Each is a compact card
  with a coloured title, live value, and a horizontal segment strip.
- Status bar: provider health dot and text, process count, generation, a record indicator,
  and a timer.
- Bottom-left: Settings and Colors buttons, with the brand wordmark above them.

### Processes

- Same shell and rail. Active destination is Processes.
- Header actions: “Process View” dropdown and a live filter field (“Filter by name, user,
  or PID”).
- Table columns: Name, PID, Status, User name, CPU usage, Memory usage. Column header
  shows the active sort.
- Rows are grouped under “Applications (20)” with expand/collapse chevrons.
- CPU usage cells include an inline horizontal bar behind the number.
- Selected row highlights; a detail panel opens below the table with four columns:
  Identity, Lifetime, Processor, Memory, plus Disk.
- Identity: parent, user, status, architecture. Lifetime: start, uptime, threads, priority.
  Processor: CPU, CPU time, GPU, NPU. Memory: footprint, private, page faults. Disk:
  reading and writing rates.
- Status bar shows provider health, process count, generation, record state and a timer.

### Theme treatment

The same layout is recoloured across Light, Dark, Green, Amber, Blue and Mono. The
phosphor themes are not just accent swaps: the graph fills, LED segments and glows change
hue, while surfaces stay dark. Light mode is a full light surface treatment, not an
inverted graph.

## 4. Feature inventory to implement

### Core measurement

- CPU: package and per-logical-processor utilisation, kernel/user split, frequency,
  topology, performance/efficiency cores where exposed, load average.
- Memory: total, used, available, cached, buffers, compressed, swap, pressure.
- Processes: PID, parent, name, command line, user, status, CPU %, CPU time, memory
  (working set/private/footprint), threads, priority, start time, architecture, disk I/O,
  GPU/NPU where available.
- Storage: per-device and aggregate read/write throughput, IOPS, utilisation, capacity.
- Network: per-interface and aggregate RX/TX rates, totals, link state, addresses,
  interface kind.
- GPU: utilisation, VRAM, clock, temperature, power where providers exist.
- Energy: instantaneous watts, battery state, charge, time remaining, per-process energy
  where the OS exposes it.
- Thermals: CPU/GPU temperatures, fans, thermal pressure, sensor inventory.
- System: OS, kernel, hostname, model, CPU, memory, boot time, uptime.

### Interaction

- Sort/filter/tree process table, follow mode, multi-select, copy.
- End task, force kill, suspend/resume, set priority — all identity-safe and permission-aware.
- Run new task.
- Startup app enable/disable.
- Service start/stop/restart and startup type, where permitted.
- Connection inspection with owning process.
- Installed-app inventory and uninstall entry points.
- Disk-space scan with largest folders/files.
- Benchmarks: CPU, memory, disk, GPU/network where meaningful; local only.
- Flight Recorder: record, save, open, scrub, replay.
- Snapshots: JSON/CSV/Markdown export and a `--snapshot` terminal mode.

### Presentation

- Twelve destinations, command palette, keyboard shortcuts.
- Light, Dark, Green, Amber, Blue, Mono, plus Red/Purple accents.
- LED segment meters, grid graphs with glow, history gaps.
- 100–200% text scale, high contrast, reduced motion.
- Tray/menu-bar HUD (optional, off by default).
- Portable mode.

## 5. Platform reality check

These constraints shape the plan and must be visible in the UI as availability states,
not worked around with invented values.

### Windows

- CPU, memory, process, disk and network data are broadly available through PDH, Toolhelp,
  `GetProcessTimes`, `GetProcessMemoryInfo`, `GetAdaptersAddresses` and IP Helper.
- Per-process GPU usage needs GPU Engine performance counters; support varies by driver.
- Thermal zone data via WMI is frequently absent or stale on desktops.
- Service control and other-user process actions require elevation.
- Installer signing needs an Authenticode certificate; unsigned builds trigger SmartScreen.

### macOS

- Mach `host_statistics`, `sysctl`, `libproc` and `getifaddrs` cover CPU, memory, process
  and network basics.
- Cross-user process CPU/energy is restricted; `task_for_pid` is denied and the
  `com.apple.system-task-ports.read` entitlement is not granted to third-party apps.
- Apple silicon temperature, fan, power and frequency data come from SMC/IOHID/IOReport,
  which are private interfaces. They can work without root but are not stable public API.
  The safe design is a provider that may report **unsupported** or **permission denied**,
  with thermal pressure from `ProcessInfo.thermalState` as the dependable fallback.
- Notarization requires an Apple Developer account and Developer ID certificate.

### Linux

- `/proc` and `/sys` cover most metrics; `hwmon`, `powercap`/RAPL, `power_supply` and
  `/proc/pressure` add sensors and pressure where present.
- Service and user management should degrade gracefully when systemd/logind are absent.
- Packaging must account for glibc/Qt versions; AppImage is the portable baseline.
- SMART and some sensors need privileges or specific device permissions.

## 6. Naming, licensing and legal notes

- Requested product name: **Task Manager OG by Don PELY**.
- Project licence: **MIT**, copyright Don Pely, 2026.
- The name contains another product's mark. The README states clearly that the project is
  independent and not endorsed. This is a product-risk item to review before wide release;
  it does not block development.
- The reference repository is MIT, but we are not copying its code or assets. The official
  application is proprietary; only public behaviour and public screenshots were studied.
- Qt will be dynamically linked under LGPLv3 with notices and replaceable libraries.
- Do not bundle the official screenshots, logo, fonts or icons in this repository.

## 7. Open questions and unverified details

1. There is no public official manual enumerating conventional menu-bar commands.
2. Exact behaviour of some Pro pages (Benchmarks methodology, Disk Space scan rules) is
   only described at marketing level.
3. Sensor coverage differs per machine; “parity” must mean parity of information
   architecture and availability semantics, not identical numbers.
4. The original's internal trace format is undocumented; ours will be an open, versioned
   format rather than a reverse-engineered clone.
5. Signing identities, Apple Developer membership and a Windows certificate are not yet
   available. Unsigned builds are the initial release path.

## Sources

- Reference repository: <https://github.com/mikedesu/evildojo-tmog>
- Reference README: <https://github.com/mikedesu/evildojo-tmog/blob/master/README.md>
- Reference plan: <https://github.com/mikedesu/evildojo-tmog/blob/master/PLAN.md>
- Reference recon notes: <https://github.com/mikedesu/evildojo-tmog/blob/master/docs/RECON.md>
- Reference window source: <https://github.com/mikedesu/evildojo-tmog/blob/master/src/gui/main_window.cpp>
- Official product page: <https://tmog.org/>
- Official release notes: <https://tmog.org/release-notes.html>
- Official Pro page: <https://tmog.org/pro.html>
- Official privacy page: <https://tmog.org/privacy.html>
- Official Linux notes: <https://tmog.org/linux.html>
- Qt release policy: <https://doc.qt.io/qt-6/qt-releases.html>
- Qt licensing: <https://doc.qt.io/qt-6/licensing.html>
- Qt LGPL obligations: <https://www.qt.io/development/open-source-lgpl-obligations>
- Qt CMake deployment: <https://doc.qt.io/qt-6/cmake-deployment.html>
