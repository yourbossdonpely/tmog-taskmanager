# Third-party notices

## Qt

This application uses the Qt 6 framework, dynamically linked, under the
**GNU Lesser General Public License, version 3 (LGPLv3)**.

Qt is copyright The Qt Company Ltd. and its contributors. Qt source code and licence
texts are available from <https://www.qt.io/> and
<https://code.qt.io/cgit/qt/qt.git/>.

Because Qt is dynamically linked and not modified:

- the application's own source remains under the MIT licence;
- users may replace the Qt libraries shipped with the application with compatible
  versions of their own;
- this notice and the LGPLv3 text are distributed with every binary package;
- no technical measure prevents replacing the Qt libraries.

Bundled Qt libraries, their version, and the exact build configuration are listed in
each release's `THIRD-PARTY.md` and SBOM. Nothing in this repository statically links
Qt.

The release pipeline will verify these obligations before publishing a package. This
document is an engineering notice, not legal advice.

## Fonts

Planned UI fonts are open-source (SIL Open Font License). Their licence files will be
included in the packaged application when the fonts are added. No fonts from the
reference application are used.

## Icons

Icons are either drawn for this project or taken from an MIT-licensed icon set, with
attribution recorded here before release. No icons or artwork from Task Manager OG are
redistributed.

## Reference application

Task Manager OG / TMOG is a product of Dave Plummer / Plummers' Software LLC. This
project is independent and is not affiliated with or endorsed by them. Only public
behaviour, public documentation and public screenshots were studied. No upstream code
or assets are included.
