# Personal remaster (modified edition)

Original game: Andrew Welch / Ambrosia Software.
Original artwork: Ian Gilman and Mark Lewis. SDL port: Sam Lantinga.
Upstream: https://github.com/libsdl-org/Maelstrom
Original artwork license: Creative Commons Attribution 3.0 Unported,
https://creativecommons.org/licenses/by/3.0/ . See ../../COPYING.
Original source license and notices remain unchanged.

## Artwork and attribution

Images/Maelstrom_Titles#130.png, ship-source.png, asteroid-source.png,
enemy-purple.png, enemy-green.png, mine.png, pickup.png and bonus.png are
AI-assisted reinterpretations made for and approved by the personal-edition
owner. They are modified artwork, not an original Ambrosia release or an
endorsement by the original creators. Retain the original attribution and
CC BY 3.0 notice with derivative copies. The original images remain intact
in Data/Images. New code-native shields, effects and icons are explicitly
personal-remaster additions, not original artwork.

Abel Regular is bundled under SIL OFL 1.1, with its notice in Fonts/OFL.txt.
stb_truetype is bundled with its original MIT/public-domain license in
game/personal-font/stb_truetype.h in the source tree (see game/personal-font/README.md). No font modifications were made.

## Run modes

- `Maelstrom --modern-art --source-sprites`: remaster UI, typography, approved
  source-art sprites and procedural display effects.
- `Maelstrom --modern-art`: modern UI/title and procedural display effects,
  without enabling the source-art sprite path.
- `Maelstrom --classic-art`: original art/UI; takes priority over modern flags.

Changes are display-only: original collision masks, sprite CRC inputs,
logical dimensions, frame counts/order, scoring and gameplay rules remain.
Source-art rotation is planar, not newly authored 3D tumbling animation.
Steel asteroids, damaged ships, multiplayer variants and some miscellaneous
art remain original. Missing optional art falls back to original visuals.

See PERSONAL-REMASTER.md at the source root for build, verification and limits.
