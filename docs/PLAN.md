# Implementation plan

**Goal:** ship `Task Manager OG by Don PELY` as a native, MIT-licensed task manager for
Windows, macOS and Linux, with signed/unsigned release artifacts and a repeatable pipeline.

**Architecture:** C++20 measurement core + Qt 6 Widgets shell, one codebase, three platform
provider sets. See [DESIGN.md](DESIGN.md).

**Status:** Phase 0 (repository and planning) is landing. No application code yet.

## Global constraints

- Product display name everywhere: **Task Manager OG by Don PELY**.
- Binary/package identifier: `task-manager-og` (Windows executable `TaskManagerOG.exe`).
- Bundle id: `com.donpely.taskmanagerog`.
- Licence: MIT, © 2026 Don Pely. Qt dynamically linked under LGPLv3 with notices.
- Minimum: C++20, CMake 3.24, Qt 6.12 LTS.
- No telemetry, accounts, licence gate or background network calls in v1.
- Every metric carries an explicit availability state; missing data is never zero.
- Every process action validates PID + creation identity.
- No upstream code, artwork, fonts or screenshots are committed.
- Releases are tag-driven; publishing a GitHub Release is a manual approval step.

---

## Phase 0 — Repository and planning

**Deliverable:** public repository with licence, README, research, design, plan, and issue
labels/milestones ready for work.

- [x] Research the reference repository and official product.
- [x] Capture the twelve destinations, UI patterns and platform constraints.
- [x] Choose the stack and record alternatives.
- [x] Write README, MIT licence, third-party notice.
- [x] Write research, design and implementation plan.
- [x] Create the public GitHub repository and push the initial commit.
- [x] Create milestones `v0.1` … `v1.0` and one tracked issue per phase.
- [ ] Protect `main`: require a green CI check once the workflow exists.

**Acceptance:** repository is public, MIT-labelled, renders the README correctly, and has no
build artifacts or secrets committed.

---

## Phase 1 — Foundation shell

**Deliverable:** an app that launches on all three platforms, shows the branded shell, and
persists settings.

- [x] `CMakeLists.txt` with C++20, warnings, and version from one place.
- [x] `src/app/main.cpp`: `QApplication`, high-DPI defaults, CLI parser, smoke and screenshot modes.
- [ ] Single-instance guard.
- [x] `src/ui/MainWindow`: left rail, destination stack, status bar, exact window title.
- [x] `src/ui/theme`: token model and Light/Dark/Green/Amber/Blue/Mono themes.
- [x] `src/core/settings`: versioned JSON, atomic save, portable mode detection.
- [x] Placeholder pages for all twelve destinations, each with a title and empty state.
- [x] Command palette shell (Ctrl/Cmd+K) and keyboard navigation.
- [x] Native application menu (About, Settings, Quit, Copy, Refresh, Help).
- [ ] Local rotating log with redaction defaults.
- [x] Console companion `tmog-cli` for version output on Windows.
- [x] `.github/workflows/ci.yml`: Windows/macOS/Ubuntu configure, build, test — green on all three.
- [x] Unit tests for settings round-trip, theme tokens and version identity.
- [x] GUI smoke test using the offscreen platform.

**Acceptance:** `cmake --build` succeeds on all three CI runners; the app opens to Summary;
the window title is exactly `Task Manager OG by Don PELY`; theme and text scale survive a
restart; `--version` prints the version.

---

## Phase 2 — Measurement core, Summary and Processes

**Deliverable:** the two hero screens with real data and safe process actions.

### Core

- [x] `Reading`, `Availability`, `MetricsSnapshot` and counter-rate math.
- [x] Sampler with configurable cadence; worker generation counter still to come.
- [x] Pure `/proc/stat`, `/proc/meminfo` and `/proc/net/dev` parsers with fixture-based tests.
- [ ] Provider registry and full health report.
- [x] Unit tests for availability, first-sample state, zero-delta guards, counter reset and a live sampler.

### Providers (all three platforms)

- [x] System-wide CPU utilisation deltas.
- [ ] CPU per-logical-processor utilisation and frequency.
- [x] Memory total/used/available.
- [ ] Memory cached/buffers/swap composition.
- [ ] Process enumeration: PID, PPID, name, user, status, CPU time, memory, threads,
  start time, architecture, command line.
- [ ] Process tree and sorting/filtering model.
- [ ] System identity: OS, kernel, hostname, model, CPU, memory, boot time, uptime.

### UI

- [x] Summary: live CPU, memory, storage and network cards with explicit provider states.
- [ ] Summary: top processes and mini panels.
- [x] Performance: CPU, memory, network history and storage capacity with gaps for missing samples.
- [ ] Performance: per-subsystem selectors, per-processor detail and dual-axis CPU graph.
- [ ] Processes: grouped tree, expand/collapse, filter, sort, selection by identity,
  detail strip, copy.
- [x] Status bar with provider health and sampling cadence.
- [ ] Status bar process count, generation and sample age.
- [ ] Process actions: end, force kill, suspend/resume, priority; confirmation and errors.
- [ ] Action audit log and identity re-validation.

### CLI

- [ ] `--snapshot`: one terminal snapshot.
- [ ] `--self-test`: per-provider report; unsupported hardware is a warning.
- [ ] `--smoke-gui`: construct and render every page offscreen.

**Acceptance:** on each platform, Summary and Processes show live values; pulling a fixture
or killing a provider turns the affected values into an explicit unavailable state with a
gap in history; terminating a process with a recycled PID is refused; `--self-test` passes
on CI runners (sensor warnings allowed).

---

## Phase 3 — Performance, power and thermal pages

**Deliverable:** the measurement pages, with parity of semantics across platforms.

- [ ] Performance landing: CPU, Memory, Disks, Network, GPU selectors.
- [ ] CPU: overall, per-logical-processor, NUMA where available, performance/efficiency
  core distinction, frequency, load.
- [ ] Memory: utilisation, pressure, composition bar, swap, history.
- [ ] Disks: per-device and combined, read/write rates, IOPS, utilisation, capacity, model.
- [ ] Network: per-interface and combined, RX/TX rates, totals, link state, addresses.
- [ ] GPU: utilisation, VRAM, clock, temperature, power where providers exist.
- [ ] Power & Freq: watts, battery state/charge/time, frequency, per-process energy where
  exposed.
- [ ] Thermals: CPU/GPU temperatures, fans, thermal pressure, sensor inventory.
- [ ] Popout graph windows and Always on Top.
- [ ] Availability matrix documented per platform and checked by tests.
- [ ] Performance budget measurement recorded in CI logs.

**Acceptance:** each page renders real data or an explained unavailable state; graphs stop
at gaps; the app stays under the documented CPU and memory budget at 1 s cadence.

---

## Phase 4 — System management

**Deliverable:** System Info, Startup apps, Users and Services.

- [ ] System Info: OS build, kernel, firmware, motherboard, CPU topology, memory modules,
  GPUs, disks, network adapters, uptime.
- [ ] Startup apps: enumerate per platform, enable/disable, show scope and publisher.
- [ ] Users: local users and active sessions, session type, login time.
- [ ] Services: enumerate, state, startup type, start/stop/restart where permitted.
- [ ] Permission model: disabled controls explain why; no silent elevation.
- [ ] Confirmation dialogs for service and startup changes.
- [ ] Audit log for every mutation.
- [ ] Tests with fixture service/user inventories and denied-permission cases.

**Acceptance:** listings match native tools on a test machine; changes require confirmation;
denied operations produce a specific message and leave state unchanged.

---

## Phase 5 — Depth tools and Flight Recorder

**Deliverable:** Connections, Installed Apps, Disk Space, Benchmarks and replay.

- [ ] Connections: TCP/UDP endpoints, state, owning process, exposure summary; no payload
  capture.
- [ ] Installed Apps: inventory with size where available, uninstall entry point with
  confirmation.
- [ ] Disk Space: scan a chosen root, largest folders/files, treemap or bar view, cancel and
  resume, scan history.
- [ ] Benchmarks: CPU, memory, disk and GPU microbenchmarks; local only; no upload; results
  stored locally; clear methodology text.
- [ ] Flight Recorder: record, save, open, scrub, replay; `.tmx` schema documented.
- [ ] Snapshot export: JSON, CSV and Markdown, with redaction options.
- [ ] Trace validation: schema version, size limit, no path traversal, no code execution.
- [ ] Cause correlation timeline: aligned CPU/memory/disk/network/thermal events with the
  responsible process (our differentiator).
- [ ] Tests for trace round-trip, corrupt traces, cancellation and redaction.

**Acceptance:** a recorded trace can be reopened on another platform and replayed with the
same panels; a corrupt trace is rejected without crashing; exports contain no command lines
or user names by default.

---

## Phase 6 — Packaging and release pipeline

**Deliverable:** installable artifacts for all three platforms and a draft GitHub Release.

- [ ] Windows: Inno Setup script, app manifest, icon, portable ZIP, uninstall entry.
- [ ] macOS: `.app` bundle, `Info.plist`, DMG layout, hardened runtime entitlements.
- [ ] Linux: `.desktop`, AppImage recipe, `.deb` metadata, `.tar.gz`.
- [ ] Exact product name asserted in installer metadata and window title.
- [ ] `release.yml`: tag `v*` builds all artifacts, generates `SHA256SUMS.txt` and SBOM,
  creates a draft release.
- [ ] Signing hooks: optional Windows Authenticode, macOS Developer ID + notarization.
- [ ] Release notes template with platform versions, checksums, known limitations.
- [ ] Unsigned-build labelling and Gatekeeper/SmartScreen instructions.
- [ ] Third-party notices and Qt LGPL text included in every package.
- [ ] Install/uninstall smoke tests on clean VMs.

**Acceptance:** a tagged pre-release installs and uninstalls cleanly on Windows 11, macOS 14+
and Ubuntu 22.04+; artifacts and checksums match; no signing secret is required to build.

---

## Phase 7 — Hardening and v1.0.0

**Deliverable:** stable release.

- [ ] Accessibility audit: keyboard-only pass, screen reader pass, 200% text, high contrast.
- [ ] Soak test: 24 hours at 1 s cadence without leaks or unbounded growth.
- [ ] Crash and hang handling: watchdog, local report, no silent data loss.
- [ ] Performance pass against the documented budget.
- [ ] Security review: file parsing, elevation boundary, command execution, export redaction.
- [ ] Documentation: user guide, provider availability matrix, trace format, build guide,
  contributing guide, code of conduct.
- [ ] Reproducible release from a clean clone; checksums verified.
- [ ] `v1.0.0` GitHub Release published manually.

**Acceptance:** release checklist complete; no open release-blocking issues; checksums and
notices verified; owner approves publication.

---

## Immediate next work (Phase 1, in order)

1. Add the CMake project skeleton and a single `main.cpp` that opens the branded window.
2. Install Qt 6.12 locally into `.tools/Qt` and verify a Windows build.
3. Add the CI matrix and get a green build on all three runners.
4. Build the theme and navigation shell, then the settings store.
5. Add the twelve placeholder destinations and the command palette shell.

## Release checklist (every version)

- [ ] Version bumped in one place; tag matches.
- [ ] CI green on Windows, macOS and Ubuntu.
- [ ] Unit, golden, CLI self-test and GUI smoke tests pass.
- [ ] Product name check passes in every installer.
- [ ] Artifacts generated for every advertised architecture.
- [ ] `SHA256SUMS.txt` generated and verified.
- [ ] `THIRD-PARTY.md`, Qt LGPL notice and MIT licence present in each package.
- [ ] Signing status stated in the release notes.
- [ ] Install, launch, upgrade and uninstall tested on clean systems.
- [ ] No secrets, symbols or build caches in the release.
- [ ] Draft reviewed and published by the owner.
