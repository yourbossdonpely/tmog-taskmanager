# Product and technical design

**Working product name:** Task Manager OG by Don PELY
**Short binary name:** `task-manager-og` / `TaskManagerOG.exe`
**Bundle identifier:** `com.donpely.taskmanagerog`
**Licence:** MIT
**Version:** 0.1.0 (pre-alpha)

## 1. Intent

Build the task manager people open when the computer feels wrong. In one glance it should
name the responsible process and subsystem. When a number cannot be measured, it should say
so. It should be native, fast and private, and it should run on Windows, macOS and Linux
without three separate products drifting apart.

Success looks like:

- A user can install it on any of the three platforms and get the same information
  architecture and vocabulary.
- A slowdown can be found, attributed and — with Flight Recorder — replayed.
- The app's own overhead is measurable and small enough that monitoring it does not change
  the answer.
- Every release is reproducible from a tagged commit, with checksums and a clear changelog.

### Goals

1. Full parity across the twelve destinations on all three desktop platforms, with explicit
   availability differences.
2. Native-feeling UI with no browser runtime.
3. Correct, testable measurement core with fixture-based tests.
4. Safe process and service actions.
5. Free and open source, with no account, telemetry or licence gate.
6. One-command builds locally and a tag-driven release pipeline.

### Non-goals for v1

- Mobile, web or remote monitoring.
- Antivirus, firewall or endpoint-security features.
- Cloud sync, accounts or a hosted dashboard.
- Kernel drivers, overclocking, fan control or other hardware manipulation.
- Automatic background updates in the first release (manual GitHub Releases download only).
- Localisation beyond English in v1; the UI will be string-extracted so translation can
  follow without rework.
- Copying the reference application's artwork, fonts, logo or exact pixel layout.

### Anti-goals

- Showing a zero because a sensor is missing.
- Hiding a permission failure behind an empty table.
- Killing a process because a PID was reused.
- Shipping a web view and calling it native.
- Shipping a paid tier in an MIT repository.
- Sending machine data anywhere by default.

## 2. People and jobs

**The developer on a laptop.** A build fans up, battery drains, the fans spin. They need the
process and the subsystem, fast, and a way to prove it after the spike is gone.

**The support technician.** They need a snapshot of a machine's state that can be attached
to a ticket without leaking command lines, user names or file paths. Export must be
reviewable and redactable.

**The power user.** They want a process tree, priorities, services, startup entries,
connections and disk usage in one consistent tool.

**The accessibility user.** They need keyboard access, scalable text, high contrast and
screen-reader labels on every control and graph summary.

## 3. Information architecture

A persistent left rail lists destinations. It is grouped so scanning is faster than a flat
list of twelve:

```
OBSERVE
  Summary
  Performance
  Processes
SYSTEM
  System Info
  Startup apps
  Users
  Services
POWER & NETWORK
  Power & Freq
  Connections
STORAGE & APPS
  Installed Apps
  Disk Space
TOOLS
  Benchmarks
  Flight Recorder
```

- `Summary` is the landing destination.
- `Flight Recorder` is a destination that can also be opened as a replay workspace from a
  saved trace.
- Settings and Colors are pinned to the bottom of the rail, as in the reference, but
  Settings is also in the native application menu (Ctrl/Cmd+,).
- The rail collapses to icons below a width threshold and can be toggled.
- A command palette (Ctrl/Cmd+K) searches destinations, actions and processes.

A native application menu provides platform conventions: About, Settings, Check for
Updates (manual, optional), Hide/Quit, Edit (Copy/Select All), View (Refresh, Theme, Text
Size, Always on Top), Tools (Run new task, Flight Recorder, Export snapshot), Window, Help.
This is our own menu design; see the research note on unverified upstream menus.

## 4. Visual language

**Concept: a precision instrument, not a dashboard.** Dark graphite surfaces, thin rules,
monospaced numerals, phosphor traces and segmented meters. Information density is high but
aligned to a strict grid.

### Palette

| Token | Dark | Light | Use |
| --- | --- | --- | --- |
| `surface` | `#0b0e13` | `#f4f5f7` | Window background |
| `panel` | `#11161d` | `#ffffff` | Cards |
| `panelRaised` | `#161d26` | `#eef1f5` | Hover / selected shell |
| `line` | `#24303c` | `#d7dce3` | Borders, grid |
| `text` | `#e7edf5` | `#10151c` | Primary |
| `textMuted` | `#8b9aab` | `#5a6675` | Labels |
| `accent` | `#5b8cff` | `#2f5fd0` | Selection, focus |
| `cpu` | `#69e06b` | `#1c8a3a` | CPU / utilisation |
| `memory` | `#d178ff` | `#8a2bb8` | Memory |
| `disk` | `#59d98e` | `#177a4a` | Storage |
| `network` | `#5b9dff` | `#235fbf` | Network |
| `energy` | `#ffe066` | `#9a7500` | Energy / battery |
| `thermal` | `#ff9d5c` | `#b34d00` | Thermals |
| `gpu` | `#7fa7ff` | `#3559b8` | GPU |
| `danger` | `#ff6b6b` | `#c02626` | Errors, force actions |

Domain colours are semantic and remain distinguishable in every theme. They are not the
only carrier of meaning: every coloured state also has a label or icon.

### Type

- Display/headings: **Space Grotesk** (SIL OFL).
- UI text: **Inter** (SIL OFL).
- Numerals, tables, traces: **IBM Plex Mono** (SIL OFL).

Fallbacks: system UI and system monospace. No fonts from the reference are used. All three
fonts are bundled with their licence files when added.

### Shape and motion

- 10 px panel radius, 6 px control radius, 1 px borders.
- 8 px spacing base; 4 px for dense table cells.
- Graphs: 1.5 px traces with a low-alpha 6 px glow, quarter grid, right-aligned newest
  sample, gaps drawn as gaps.
- Meters: 18–24 segments, vertical or horizontal, dim segments visible, lit segments glow.
- Motion: 120–180 ms fades for theme changes and panel reveal. A reduced-motion setting
  disables glow animation and trace interpolation. Graphs update at the sample cadence, not
  at an artificial 60 fps, except the optional smooth mode.

### Accessibility

- Text scale 100–200% with layout reflow, not clipping.
- System high-contrast palette detection and a manual override.
- Every graph exposes a textual summary and its latest values to screen readers.
- Full keyboard traversal, visible focus ring, no colour-only status.
- Minimum 24×24 px targets, 32×32 for primary actions.
- Status changes are announced through accessible live regions where the platform supports
  it.

## 5. Architecture

### Stack decision

**C++20 + Qt 6.12 Widgets, CMake, dynamically linked Qt.**

Why:

- One UI codebase for all three desktop platforms, with native widgets rather than a browser.
- Direct access to OS APIs and a small memory/CPU footprint, which matters when the app is
  measuring itself.
- Mature tables, models, accessibility, high-DPI handling, tray icons and native menus.
- CMake deployment and packaging support on all three platforms.

Alternatives considered:

| Option | Why not |
| --- | --- |
| Three native shells (Win32 + AppKit + Qt) over a shared core | Highest fidelity to the original, but three UI implementations, three skill sets and three times the maintenance. Not viable for the first release. |
| Electron or Tauri | Contradicts “native, no web view”; a monitor that carries a browser runtime distorts its own measurements and bloats installers. |
| Rust + egui | Small and fast, but table, accessibility and native-dialog maturity are weaker for an information-dense desktop tool. |
| .NET + Avalonia | Viable and productive, but a bundled runtime adds size and self-measurement noise; C++ gives direct provider control. |

### Layers

```
┌────────────────────────────────────────────────────────────┐
│ ui/         MainWindow, destination pages, widgets, theme  │
├────────────────────────────────────────────────────────────┤
│ app/        Application, command line, tray, actions       │
├────────────────────────────────────────────────────────────┤
│ core/       Metric<T>, Snapshot, History, Trace, Settings  │
├────────────────────────────────────────────────────────────┤
│ platform/   Provider interfaces + win32/ macos/ linux/     │
└────────────────────────────────────────────────────────────┘
```

Dependency rule: `platform` knows nothing about `ui`; `core` knows nothing about Qt
Widgets; `ui` reads immutable snapshots and sends commands. Qt Core is allowed in `core`
for containers/signals only if it does not leak into provider tests. Prefer the standard
library in `core`; keep Qt at the edges.

### Measurement model

```cpp
enum class Availability {
    Available,
    Unsupported,          // this OS/version/hardware does not expose it
    PermissionDenied,
    TemporarilyUnavailable,
    Error,
};

template <typename T>
struct Metric {
    std::optional<T> value;
    Availability state = Availability::Unsupported;
    std::string provider;     // e.g. "windows.pdh", "linux.proc.stat"
    std::string detail;       // human-readable reason when not Available
    std::int64_t sampledAtMs = 0;
};
```

Rules:

- `value` is only present when `state == Available`.
- A missing value never becomes `0`.
- A history ring records the availability state alongside the value, so the graph can draw a
  gap.
- Every snapshot has a monotonic `generation` and a wall-clock timestamp.
- Provider identity is retained for diagnostics and the status bar.

### Sampling

- One `Sampler` worker owns all providers and produces an immutable `Snapshot`.
- Default cadence: 1 s. Allowed: 0.5, 1, 2, 5 s. Rates are computed from wall-clock deltas,
  not from an assumed tick count.
- Slow providers (SMART, installed apps, disk scans) run on their own schedule and publish
  results into the next snapshot; they never block the fast path.
- The UI renders the latest snapshot; it never calls a provider directly.
- History capacity is time-based (e.g. 10 minutes at the configured cadence), with a hard
  memory cap per series.
- When the app is hidden/minimised, sampling may drop to a low-power cadence; history records
  the gap. This behaviour is a setting.

### Process identity and actions

```cpp
struct ProcessIdentity {
    std::uint32_t pid;
    std::uint64_t creationTime; // 100 ns ticks on Windows, start time on POSIX
};
```

Every action re-validates `(pid, creationTime)` immediately before acting. If the process
has exited or the identity changed, the action fails with a clear message and no signal is
sent. PID 0 and negative/group targets are rejected. Actions carry an audit entry in memory
and in the local log.

Action matrix:

| Action | Windows | macOS | Linux |
| --- | --- | --- | --- |
| End task | `TerminateProcess` after graceful attempt | `SIGTERM` | `SIGTERM` |
| Force kill | `TerminateProcess` | `SIGKILL` | `SIGKILL` |
| Suspend / resume | `NtSuspendProcess` (documented risk) or thread walk | `SIGSTOP`/`SIGCONT` | `SIGSTOP`/`SIGCONT` |
| Priority | `SetPriorityClass` | `setpriority` | `setpriority` |
| Other users | requires elevation | same-uid only | requires privileges |

Unsupported or denied actions are disabled with an explanation, not hidden silently.

### Platform providers

Each domain has an interface and three implementations plus a fixture implementation for
tests.

| Domain | Windows | macOS | Linux |
| --- | --- | --- | --- |
| CPU | PDH, `GetSystemTimes`, registry topology | Mach `host_statistics`, `sysctl` | `/proc/stat`, `/proc/cpuinfo`, sysfs cpufreq |
| Memory | `GlobalMemoryStatusEx`, `GetPerformanceInfo` | `host_statistics64`, `sysctl` | `/proc/meminfo`, `/proc/vmstat`, pressure |
| Processes | Toolhelp/EnumProcesses, `GetProcessTimes`, `GetProcessMemoryInfo`, `QueryFullProcessImageNameW` | `libproc`, `proc_pidinfo`, `proc_pid_rusage` | `/proc/[pid]` |
| Storage | PDH logical/physical disk, `GetDiskFreeSpaceExW` | IOKit block storage, `statfs` | `/proc/diskstats`, `/sys/class/block`, `statvfs` |
| Network | PDH, `GetAdaptersAddresses`, IP Helper tables | `getifaddrs`, `sysctl`, libproc sockets | `/proc/net/dev`, `/sys/class/net`, `/proc/net/*` |
| GPU | PDH GPU Engine, DXGI | IOKit IOAccelerator | NVML/`/sys/class/drm` where available |
| Energy | `GetSystemPowerStatus`, battery/energy counters | AppleSmartBattery, IOReport (private, optional) | `power_supply`, RAPL |
| Thermals | WMI thermal zones, PDH where present | SMC/IOHID (private, optional), `ProcessInfo.thermalState` | `hwmon`, `thermal_zone` |
| System | registry, `GetNativeSystemInfo`, WMI | `sysctl`, `uname` | `uname`, `/etc/os-release`, DMI |
| Startup | Run keys, Startup folders, Task Scheduler | launchd plists/`launchctl` | XDG autostart, systemd user units |
| Users | WTS sessions, NetUserEnum | OpenDirectory, `utmp`/`utmpx` | logind/utmp |
| Services | SCM | `launchctl` | systemd D-Bus, fallback to init scripts |
| Connections | IP Helper TCP/UDP tables + owner PID | libproc + sysctl | `/proc/net/*` + socket inodes |
| Installed apps | Uninstall registry keys | `/Applications`, receipts | dpkg/rpm/flatpak/snap |
| Disk space | directory walk with `GetDiskFreeSpaceExW` | directory walk with `NSFileManager` | directory walk with `statvfs` |
| Benchmarks | local CPU/memory/disk/GPU | local CPU/memory/disk/GPU | local CPU/memory/disk/GPU |

Private OS interfaces are isolated behind a provider and are allowed to fail. They must
never be required for the app to launch or for core pages to render.

### History and Flight Recorder

- `HistoryRing<T>` stores `(timestamp, optional<T>, Availability)`.
- Flight Recorder records the same snapshot stream at a chosen cadence into a versioned
  trace: a JSON manifest plus JSONL sample records, packaged in a zip container with the
  extension `.tmx`.
- The manifest records app version, platform, provider inventory, cadence, start/end time,
  and a schema version. No user names, command lines or file paths are recorded unless the
  user explicitly enables “include process details”.
- Playback scrubs the recorded timeline; live pages and playback share the same rendering
  code.
- Traces are local files. Opening one never executes content.

### Settings and storage

Platform-native config directory:

- Windows: `%APPDATA%\DonPely\TaskManagerOG`
- macOS: `~/Library/Application Support/com.donpely.taskmanagerog`
- Linux: `$XDG_CONFIG_HOME/task-manager-og` (default `~/.config/task-manager-og`)

Format: versioned JSON with atomic writes (temp file + rename). Unknown keys are preserved.
Settings include theme, accent, text scale, cadence, reduced motion, start page, rail state,
column widths/sort, follow mode, tray behaviour, self-monitoring, and trace defaults.

Portable mode: if a `portable.flag` file exists next to the executable, settings and traces
go to a `data/` directory beside it. This is opt-in and documented.

### Security and privacy

- No network calls in v1 except an explicit, manual “Check for updates” that opens the
  GitHub Releases page in the browser. No automatic update check on launch.
- No telemetry, analytics, crash upload or unique identifiers.
- Logs are local, rotated, and exclude command lines, user names and file paths by default.
- Exported snapshots default to redacted process names? No: names are useful. Default export
  excludes command lines, user names, and full paths, with an option to include them.
- Actions require explicit confirmation for force-kill and service changes.
- Elevated operations are never performed by silently relaunching the whole app as admin.
  The plan is a narrow helper or a documented one-shot elevation flow, decided in the
  relevant milestone.
- Trace files are treated as untrusted input: schema validation, size limits, no code
  execution, no path traversal.

### Threading

- One sampler thread, one UI thread.
- Provider results are moved into snapshots; no provider mutates UI state.
- Slow scans run on a bounded worker pool with cancellation.
- The UI never blocks on I/O; actions are asynchronous with progress and a result message.
- Shutdown cancels scans and joins threads within a bounded timeout.

### Error handling

- Every provider returns `Metric<T>`; exceptions do not cross the provider boundary.
- The status bar shows provider health: healthy, degraded, unavailable, denied.
- A “Diagnostics” panel can copy a redacted provider report.
- Errors are logged once per state change, not every tick.

## 6. Packaging and release

### Artifacts

```
TaskManagerOG-<version>-windows-x64-setup.exe
TaskManagerOG-<version>-windows-arm64-setup.exe
TaskManagerOG-<version>-windows-x64-portable.zip
TaskManagerOG-<version>-macos-universal.dmg
TaskManagerOG-<version>-linux-x86_64.AppImage
TaskManagerOG-<version>-linux-amd64.deb
TaskManagerOG-<version>-linux-x86_64.tar.gz
SHA256SUMS.txt
```

ARM64 Linux artifacts are added once the CI runner and Qt packages are proven.

### Installer requirements (exact product name everywhere)

- **Windows (Inno Setup):** wizard title, application name, Start Menu folder and shortcut
  all read `Task Manager OG by Don PELY`. Licence page shows the MIT licence. User can
  choose the install directory. Default `C:\Program Files\Task Manager OG by Don PELY`.
  Uninstall entry uses the full name. Optional “Launch after install”. No bundled toolbars
  or third-party offers.
- **macOS:** `Task Manager OG by Don PELY.app` in a DMG with an Applications shortcut.
  `CFBundleDisplayName` is the full name; `CFBundleName` is the shorter `Task Manager OG`
  because macOS truncates the app menu, and the About panel plus window title show the full
  name. Drag-to-Applications layout. Signed and notarized when credentials exist.
- **Linux:** `.desktop` `Name=Task Manager OG by Don PELY`, `Comment=Native system monitor`,
  `Exec=task-manager-og`, `Icon=task-manager-og`. AppImage and `.deb` install the desktop
  entry and icon. Category `System;Monitor;`.

The exact name is a release-blocking check in CI: the installer metadata and the running
window title are asserted in a packaging test.

### Versioning

- Semantic versioning, tags `vMAJOR.MINOR.PATCH`.
- `v0.x` is pre-release; artifacts are marked as pre-release on GitHub.
- `v1.0.0` is the first stable release.
- The version is defined once in `CMakeLists.txt` and injected into the app, installers and
  `--version` output.

### CI/CD

- `ci.yml`: matrix Windows/macOS/Ubuntu, configure, build, run unit tests, upload a short
  log on failure.
- `release.yml`: on tag `v*`, build release artifacts, generate checksums and an SBOM,
  create a **draft** GitHub Release, and attach artifacts. Publishing the draft is a manual,
  owner-approved step.
- Optional signing jobs run only when the relevant secrets exist; otherwise the release notes
  state that the build is unsigned.
- No release is published without the checksum file and the licence/third-party notices.

### Signing prerequisites (owner action later)

- Windows: Authenticode code-signing certificate (OV or EV) and a timestamp service.
- macOS: Apple Developer Program membership, Developer ID Application certificate,
  notarization credentials (`notarytool`).
- These are secrets, never committed. The pipeline must build unsigned successfully without
  them.

## 7. Testing

- **Unit tests** for every parser and state machine, driven by fixture files/directories.
  No root, no live system required.
- **Golden tests** for history gaps, rate calculation, counter wrap, process sorting and
  identity validation.
- **Provider smoke tests** on live systems, opt-in, reported as warnings when hardware is
  absent.
- **CLI self-test** (`--self-test`) prints a per-provider report and exits non-zero on a
  genuine failure; unsupported hardware is a warning.
- **GUI smoke test** (`--smoke-gui`, offscreen platform) constructs every page, renders a
  few generations, and exits.
- **Packaging tests** assert product name, version, desktop entry, and presence of notices.
- **Accessibility checks**: keyboard traversal, accessible names, text-scale screenshots.
- **Performance budget**: under 2% of one core at 1 s cadence on an idle 4-core machine, and
  under 150 MB resident with 10 minutes of history. Measured and reported, not assumed.

## 8. Delivery risks

| Risk | Impact | Mitigation |
| --- | --- | --- |
| macOS sensor APIs are private and may change | Thermals/power pages incomplete on some Macs | Provider isolation, thermal-pressure fallback, explicit unsupported states |
| Windows actions need elevation | Some processes cannot be managed | Clear disabled states, optional narrow helper, never silent elevation |
| Linux fragmentation | Services/users missing without systemd | Capability detection and graceful degradation |
| Qt LGPL compliance | Legal/release blocker | Dynamic linking, notices, relinkable libraries, SBOM check |
| Signing certificates unavailable | SmartScreen/Gatekeeper warnings | Unsigned pre-releases clearly labelled; signing later without code changes |
| Scope of twelve destinations | Long build | Milestones ship usable increments; Summary/Processes first |
| Product name uses another mark | Trademark risk | Non-affiliation notice; renameable without code changes |
| Self-measurement overhead | Undermines the product promise | Performance budget, sampling backoff, self-monitoring view |

## 9. Decisions recorded

1. **One Qt 6 shell, not three native shells.** Chosen for a realistic first release and
   consistent semantics. Revisit only if platform-native fidelity becomes a hard requirement.
2. **All features free.** No Pro gate, no licence server, no account.
3. **No network by default.** Manual update link only in v1.
4. **Own visual identity.** Same information architecture and instrument metaphor, new
   palette, type, spacing and brand.
5. **Open trace format.** Versioned `.tmx`, documented, no reverse-engineered upstream format.
6. **Exact display name.** `Task Manager OG by Don PELY` in installers, shortcuts, window
   title and About; short binary/package names for filesystem and CLI ergonomics.
