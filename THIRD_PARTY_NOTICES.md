# Third-party notices

The project [MIT license](LICENSE) covers original NUM GAME code, original
icons/geometry and independently generated puzzles. It does not relicense the
following components. Complete notices are retained under `docs/third_party`.

| Material | Use and terms |
|---|---|
| SOKOBAN, local reference at `4287f21df24e4d7abea06eb17a849bbbb4cb862b` | Adapted native MENU/OFF, input barrier and BFile patterns, build configuration; copied/adapted G3A verifier. MIT, copyright 2026 omegalpha210. [Notice](docs/third_party/SOKOBAN-LICENSE.txt). No maps or Sokoban engine included. |
| DIFF EQ, local reference at `1a16b1b728c7c9e8a0e2491d96fde4b95cdacdbb` | Read-only platform reference; the G3A verifier and font conversion approach descend from its MIT infrastructure. [Notice](docs/third_party/DIFFEQ-LICENSE.txt). No ODE engine/data included. |
| [gint](https://git.planet-casio.com/Lephenixnoir/gint) | External native runtime, normal `font8x9.png` and compact `font5x7.png` atlases; converted font data in app. Custom permissive [README permission](docs/third_party/gint-README.md). No proprietary CASIO font bundled. |
| [fxSDK](https://git.planet-casio.com/Lephenixnoir/fxsdk) | External build/packaging tools and format specification. [MIT notice](docs/third_party/fxSDK-LICENSE.txt). |
| [FxLibc](https://git.planet-casio.com/Vhex-Kernel-Core/fxlibc) | Native C library, [CC0 notice](docs/third_party/fxlibc-LICENSE.txt), with [Grisu2b MIT notice](docs/third_party/fxlibc-Grisu2b-LICENSE.txt) retained for separate portions. |
| OpenLibm SH port | External SDK library; [combined upstream notices](docs/third_party/OpenLibm-LICENSE.md). This app's engines use integer/rational arithmetic. |
| GCC runtime support | SH integer arithmetic/compiler support linked by GCC. [GCC Runtime Library Exception 3.1](docs/third_party/GCC-RUNTIME-EXCEPTION.txt) and [GPLv3](docs/third_party/GPL-3.0.txt). |

Game engines and AI policies are independently authored. Rule-only references
to Nikoli, Project Euler, Simon Tatham, James Harvey and Gabriele Cirulli are
listed in [asset provenance](docs/ASSET_PROVENANCE.md) and family audits. No
referenced puzzle instance, logo, screenshot or third-party game source code is
bundled. In particular, no DOS `pusher`/Sokoban maps are included.

CMake, Python, Pillow, GCC and binutils are external development dependencies;
their toolchains, caches and executables are not included in this repository.
CASIO names identify compatibility and do not imply affiliation or endorsement.
