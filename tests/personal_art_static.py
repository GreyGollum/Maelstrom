"""Non-interactive remaster asset checks; does not launch game or alter data."""
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
root = Path(__file__).resolve().parents[1]
art = root / 'Data' / 'PersonalArt'
for path in (art / 'UI').glob('*.xml'):
    ET.parse(path)
images = ['Maelstrom_Titles#130.png', 'ship-source.png', 'asteroid-source.png',
          'enemy-purple.png', 'enemy-green.png', 'mine.png', 'pickup.png', 'bonus.png']
for name in images:
    data = (art / 'Images' / name).read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', name
    width, height = struct.unpack('>II', data[16:24])
    assert 0 < width <= 4096 and 0 < height <= 4096, name
assert (art / 'Fonts' / 'Abel-Regular.ttf').read_bytes()[:4] == b'\x00\x01\x00\x00'
assert 'SIL OPEN FONT LICENSE' in (art / 'Fonts' / 'OFL.txt').read_text()
assert (root / 'game/personal-font/stb_truetype.h').is_file()
for name in ['original_credits.xml', 'original_story.xml']:
    assert (art / 'UI' / name).is_file()
print(f'PASS: {len(images)} PNG assets, UI XML, font signature and notices')
