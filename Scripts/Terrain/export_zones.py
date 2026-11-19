# 구역 이름 표시(Q130)의 데이터 파일을 레이아웃에서 만든다 — docs/systems/world-zones.md
#
#   정본: Scripts/Terrain/openbeta_layout.json 의 WorldOrigin · Sections(이름 · 설명 · 경계)
#   생성물: Content/Grimoire/Data/WorldZones.json(게임이 런타임에 읽는다 — 출시본에도 들어간다)
#
#   python Scripts/Terrain/export_zones.py           # 생성물을 다시 쓴다
#   python Scripts/Terrain/export_zones.py --check   # 생성물이 레이아웃과 같은가만 본다(다르면 종료 코드 1)
#
# 레이아웃의 Sections 를 고쳤으면 이 스크립트를 돌려 생성물을 같이 커밋한다.
# 자동화 테스트 Grimoire.World.Zones.DataMatchesLayout 이 둘이 어긋나면 실패한다.
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
LAYOUT = os.path.join(ROOT, 'Scripts', 'Terrain', 'openbeta_layout.json')
OUTPUT = os.path.join(ROOT, 'Content', 'Grimoire', 'Data', 'WorldZones.json')

# 게임이 쓰는 필드만(Height 는 하늘 글씨 몫이라 뺀다)
FIELDS = ('Name', 'Tier', 'Label', 'Subtitle', 'Note', 'Center', 'Radius', 'Polygon')


def point(p):
    return '{ "X": %s, "Y": %s }' % (fmt(p['X']), fmt(p['Y']))


def fmt(v):
    return str(int(v)) if float(v).is_integer() else repr(float(v))


def build():
    with open(LAYOUT, encoding='utf-8') as f:
        layout = json.load(f)
    sections = layout.get('Sections', [])
    names = set()
    for s in sections:
        if s['Name'] in names:
            raise SystemExit(f'구역 이름이 겹친다: {s["Name"]}')
        names.add(s['Name'])
        if s.get('Tier') not in ('Region', 'Sector'):
            raise SystemExit(f'{s["Name"]}: Tier 는 Region 또는 Sector')
        if s['Tier'] == 'Region' and len(s.get('Polygon', [])) < 3:
            raise SystemExit(f'{s["Name"]}: Region 은 Polygon(꼭짓점 3 이상)이 있어야 한다')

    # 한 구역 = 한 줄(사람이 diff 를 읽기 쉽게)
    out = ['{',
           '  "_comment": "생성물 — 고치지 말 것. 정본은 Scripts/Terrain/openbeta_layout.json 의 Sections. '
           '고친 뒤 python Scripts/Terrain/export_zones.py (docs/systems/world-zones.md)",',
           '  "WorldOrigin": %s,' % point(layout['WorldOrigin']),
           '  "Sections": [']
    rows = []
    for s in sections:
        parts = []
        for key in FIELDS:
            if key not in s:
                continue
            v = s[key]
            if key == 'Center':
                parts.append('"Center": %s' % point(v))
            elif key == 'Polygon':
                parts.append('"Polygon": [ %s ]' % ', '.join(point(p) for p in v))
            elif key == 'Radius':
                parts.append('"Radius": %s' % fmt(v))
            else:
                parts.append('%s: %s' % (json.dumps(key), json.dumps(v, ensure_ascii=False)))
        rows.append('    { ' + ', '.join(parts) + ' }')
    out.append(',\n'.join(rows))
    out.append('  ]')
    out.append('}')
    return '\n'.join(out) + '\n'


def main():
    # 한국어 Windows 콘솔(cp949)에서도 메시지가 깨지거나 멈추지 않게
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    text = build()
    json.loads(text)  # 형식 확인
    if '--check' in sys.argv:
        try:
            with open(OUTPUT, encoding='utf-8') as f:
                now = f.read().replace('\r\n', '\n')
        except FileNotFoundError:
            now = ''
        if now != text:
            print(f'어긋남: {os.path.relpath(OUTPUT, ROOT)} 가 레이아웃과 다르다 — python Scripts/Terrain/export_zones.py')
            return 1
        print('WorldZones.json 은 레이아웃과 같다')
        return 0
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)
    print(f'{os.path.relpath(OUTPUT, ROOT)} 를 썼다 — 구역 {text.count(chr(34) + "Name" + chr(34))}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
