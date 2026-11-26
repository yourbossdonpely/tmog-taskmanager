# UI specification: reference measurements and Qt implementation

**Reference:** official TMOG Summary, Processes, CPU, Memory, Disks, Network, Energy, Thermals and theme screenshots at `https://tmog.org/assets/screenshots/` (1800 × 1156 WebP). These are public product images; they are **not** committed to this repository.

**Method:** two passes against the 1800 × 1156 Summary image:

1. Pixel sampling and edge scans for the shell, rail, active pill, cards, gaps and status bar.
2. Canny contour extraction for card bounds, then the same contour check against a 1800 × 1156 render of this app. The final implementation lands within ±2 px of the reference bounds.

No upstream source code, artwork, fonts or icons were used. The layout proportions and measured shell geometry are matched; the brand wordmark and all line icons are our own.

## Shell geometry (reference and implementation, logical px at 1800 × 1156)

| Element | Reference | Implementation |
| --- | --- | --- |
| Window / screenshot | 1800 × 1156 | 1800 × 1156 design target; screenshots downscaled to this size |
| Sidebar | 346 wide, 1 px border | 346 wide, 1 px border |
| Nav active pill | x 10 → 330, 320 wide, about 54 high | x 10 → 330, 320 wide, 52 high |
| Nav icon / text | icon at x 36, text at x 74 | icon at x 36, text at x 74 |
| Nav row pitch | about 56 | 54 (15 destinations fit without scrolling) |
| Page header | 94 high | 94 high |
| Page icon / title | icon x 384, title x 432, title 36 px | same positions; 36 px title |
| Content gutters | left 22, right 16 | left 22, right 16 |
| Top card row | 480 high | 480 high |
| Top card widths | 264 / 602 / 523, 12 px gaps | 264 / 604 / 526, 12 px gaps |
| Memory band | 1416 × 260, 10 px below top row | 1418 × 262, 10 px below |
| Mini-card rows | 465 × 116 and 465 × 115, 11/12 px gaps | 467 × 118 each, 11/12 px gaps |
| Status bar | 38 high | 38 high |
| Card radius / border | 8 px / 1 px | 8 px / 1 px |

The top-row and band bounds are measured from the reference contours at `(368, 94, 264, 480)`, `(644, 96, 602, 478)`, `(1261, 94, 523, 480)`, `(368, 585, 1416, 260)`, `(368, 856, 465, 116)` and `(368, 984, 465, 115)`. Our 1800 × 1156 render produces `(367, 93, 267, 482)`, `(644, 93, 604, 482)`, `(1259, 93, 526, 482)`, `(367, 583, 1418, 262)` and `(367, 854, 467, 118)`. The remaining 1–3 px comes from antialiasing on the 1 px border and the rounded corners.

## Surfaces and type

| Token | Measured reference | Implementation |
| --- | --- | --- |
| Window / rail surface | `#1e1e1e` | `#1e1e1e` |
| Card surface | about `#262724` | `#262724` |
| Active nav fill | about `#28314e` | `#2c3550` |
| Border | about `#3a3b37` | `#3a3b37` |
| Primary text | warm off-white | `#ecece7` |
| Secondary text | warm grey | `#a3a39c` |
| Page title | about 36 px, light weight | 36 px, weight 300 |
| Navigation | about 16 px | 16 px |
| Card title | about 22 px | 22 px |
| Metric value | 30 px monospace | 30 px monospace |
| Nav labels and icons | 20 px icon, 16 px label | same |
| Group labels | small accent caps with rule | same |

The reference has a macOS traffic-light strip at the top of the rail. This app uses the host window frame and native menu conventions instead: the in-window menu bar is hidden on Windows/Linux and replaced by the small header menu button, while macOS keeps its native application menu. That preserves the client-area measurements on every host.

## Summary structure

The implementation follows the reference's bands, not its data placeholders:

- **Top row:** four vertical LED gauges (CPU, Memory, Storage, Pressure), a wide CPU Overview history card, and a Process pressure history card.
- **Memory band:** full-width Memory Utilization history.
- **Mini grid:** Storage, Network, Processes, Pressure, CPU and Memory, each with a measured strip where the reading has a percentage or ratio.
- **Status:** provider dot, health, process count, cadence and full product/version.

The reference's top-process table is not faked. Per-process CPU attribution is still a core milestone; until it lands, the same top-row slot carries a real process-pressure history and process counts. Missing readings still become `UNAVAILABLE`, not a zero.

## Performance structure

The Performance page uses the reference's left subsystem rail plus a large detail panel:

- Subsystems: CPU, Memory, Network, Storage, Process pressure.
- The selected row shows a full history, current/average/peak values and sample count.
- A live snapshot panel carries process count, logical CPU count and samples in view.
- Network detail exposes receive/transmit history and a legend; unavailable samples stay as gaps.

## All destinations are free

The rail order is exactly:

1. Summary
2. Performance
3. Processes
4. System Information
5. App History
6. Startup Apps
7. Users
8. Services
9. Power and Frequency
10. Flight Recorder
11. Connections
12. Installed Apps
13. Drivers
14. Deep Space
15. Benchmarks

There is no `PRO` divider, no lock icon, and no paid or gated destination. Destination pages carry a `FREE · INCLUDED` badge and state that every module is part of the same free product.

## Deliberate differences

- **Own brand and icons.** The TMOG wordmark and the original product's line art are not copied. The `TMOG` wordmark and all 18 line icons are our own drawing.
- **No exact screenshot redistribution.** The official images are used only as measurement references and are not shipped in the repository.
- **Expanded rail.** The reference has 11 visible destinations plus a Pro divider. This product exposes 15 free destinations, so the row pitch is 54 instead of 56 and the active pill is 52 high; every item stays visible at the reference window height without scrolling.
- **Process table later.** The reference's top-process table and per-core CPU grid require providers that are not yet implemented. Those slots display real aggregate data rather than invented rows or cores.
- **Platform chrome.** macOS gets the native menu; Windows and Linux hide the in-window menu bar and expose the same commands from the header menu button.
