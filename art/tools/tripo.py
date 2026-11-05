"""Tripo API(그림 → 3D) 호출 — 업로드 · 생성 · 기다림 · 결과 내려받기를 한 번에.

    python art/tools/tripo.py balance
    python art/tools/tripo.py image <그림.png> <결과 폴더> [--fast]
    python art/tools/tripo.py multiview <정면.png> <왼쪽.png|-> <뒤.png|-> <오른쪽.png|-> <결과 폴더> [--fast]
    python art/tools/tripo.py sheet <4면 시트.png> <결과 폴더> [--fast]

- 멀티뷰는 정면 · 왼쪽 · 뒤 · 오른쪽 4장까지다(위 · 아래는 Tripo 가 받지 않는다). 정면은 꼭 있어야 한다.
- sheet: GPT 가 한 장에 2×2 로 그린 4면 시트(왼쪽 위 정면 · 오른쪽 위 왼쪽 · 왼쪽 아래 뒤 · 오른쪽 아래 오른쪽)를
  잘라 <결과 폴더>/views/ 에 두고 멀티뷰로 만든다. 따로 그린 4장은 서로 디테일이 어긋나서 한 장에 그리게 한다.
  자르기에 Pillow 가 필요하다(pip install --user pillow) — 그래서 python -I 로 부르지 않는다.

- 키는 환경 변수 TRIPO_API_KEY, 없으면 ~/.tripo/api_key 파일에서 읽는다. 키는 화면 · 로그 · 결과 파일 어디에도 쓰지 않는다.
- 결과 링크는 5분 뒤 만료된다 — 성공하면 바로 모든 결과 파일(GLB · 미리보기 그림)을 내려받는다.
- 결과 폴더에 task.json(요청 값 · 작업 번호 · 상태)을 남겨 같은 에셋을 다시 만들 때 쓴다.
- 기본은 최고 품질(기하 detailed · 텍스처 detailed · 굽힌 빛 지우기). --fast 는 시험용(standard).
"""
import json
import mimetypes
import os
import sys
import time
import urllib.error
import urllib.request
import uuid

API = "https://openapi.tripo3d.ai/v3"
BALANCE_URL = "https://api.tripo3d.ai/v2/openapi/user/balance"
MODEL = "v3.1-20260211"
TEXTURE_VERSION = "v3.5-20260815"  # delight(그림에 구워진 빛 지우기)는 이 텍스처 버전에서만 된다
POLL_SECONDS = 3
TIMEOUT_SECONDS = 900


def api_key():
    key = os.environ.get("TRIPO_API_KEY", "").strip()
    if not key:
        path = os.path.expanduser("~/.tripo/api_key")
        if not os.path.exists(path):
            sys.exit("키가 없다: 환경 변수 TRIPO_API_KEY 또는 ~/.tripo/api_key")
        with open(path, encoding="ascii") as f:
            key = f.read().strip()
    return key


def request(method, url, body=None, content_type="application/json"):
    headers = {"Authorization": "Bearer " + api_key()}
    data = None
    if body is not None:
        data = body if isinstance(body, bytes) else json.dumps(body).encode()
        headers["Content-Type"] = content_type
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            result = json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        sys.exit(f"{method} {url} → HTTP {e.code}: {e.read().decode()[:400]}")
    if result.get("code") != 0:
        sys.exit(f"{method} {url} → {json.dumps(result, ensure_ascii=False)[:400]}")
    return result["data"]


def balance():
    data = request("GET", BALANCE_URL)
    print(f"크레딧 잔액 {data.get('balance')} (보류 {data.get('frozen')})")
    return data


def upload(path):
    boundary = uuid.uuid4().hex
    mime = mimetypes.guess_type(path)[0] or "application/octet-stream"
    with open(path, "rb") as f:
        payload = f.read()
    body = (
        f"--{boundary}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"{os.path.basename(path)}\"\r\n"
        f"Content-Type: {mime}\r\n\r\n"
    ).encode() + payload + f"\r\n--{boundary}--\r\n".encode()
    token = request("POST", API + "/files", body, f"multipart/form-data; boundary={boundary}")["file_token"]
    print(f"업로드 {os.path.basename(path)} → {token}")
    return token


def quality(fast):
    if fast:
        return {"texture_quality": "standard", "geometry_quality": "standard"}
    return {"texture_quality": "detailed", "geometry_quality": "detailed"}


def wait(task_id):
    start = time.time()
    last = None
    while time.time() - start < TIMEOUT_SECONDS:
        data = request("GET", f"{API}/tasks/{task_id}")
        status, progress = data.get("status"), data.get("progress")
        if (status, progress) != last:
            print(f"  {status} {progress}%  ({time.time() - start:.0f}s)")
            last = (status, progress)
        if status == "success":
            return data
        if status in ("failed", "cancelled", "banned", "expired", "unknown"):
            sys.exit(f"작업 실패: {json.dumps(data, ensure_ascii=False)[:400]}")
        time.sleep(POLL_SECONDS)
    sys.exit(f"작업이 {TIMEOUT_SECONDS}s 안에 끝나지 않았다: {task_id}")


def download_outputs(data, out_dir):
    saved = []
    for name, url in (data.get("output") or {}).items():
        if not isinstance(url, str) or not url.startswith("http"):
            continue
        ext = os.path.splitext(url.split("?")[0])[1] or ".bin"
        path = os.path.join(out_dir, f"{name}{ext}")
        with urllib.request.urlopen(url, timeout=300) as r, open(path, "wb") as f:
            f.write(r.read())
        saved.append(path)
        print(f"  받음 {path} ({os.path.getsize(path) // 1024} KB)")
    return saved


def generate(endpoint, params, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    task_id = request("POST", f"{API}/generation/{endpoint}", params)["task_id"]
    print(f"생성 요청 {endpoint} → 작업 {task_id}")
    record = {"endpoint": endpoint, "params": params, "task_id": task_id}
    with open(os.path.join(out_dir, "task.json"), "w", encoding="utf-8") as f:
        json.dump(record, f, ensure_ascii=False, indent=2)
    data = wait(task_id)
    record["files"] = download_outputs(data, out_dir)
    record["consumed_credit"] = data.get("consumed_credit")
    with open(os.path.join(out_dir, "task.json"), "w", encoding="utf-8") as f:
        json.dump(record, f, ensure_ascii=False, indent=2)
    balance()


VIEW_KEYS = ("front", "left", "back", "right")


def upload_views(paths):
    """[정면, 왼쪽, 뒤, 오른쪽] 경로('-' 는 건너뜀) → Tripo 의 보기 키 입력"""
    if paths[0] == "-":
        sys.exit("정면 그림은 꼭 있어야 한다")
    return [{key: upload(path)} for key, path in zip(VIEW_KEYS, paths) if path != "-"]


def runs(flags):
    """참 값이 이어진 구간들 [(시작, 끝+1), …]"""
    found, start = [], None
    for i, flag in enumerate(list(flags) + [False]):
        if flag and start is None:
            start = i
        elif not flag and start is not None:
            found.append((start, i))
            start = None
    return found


def isolate(band, center_x):
    """띠(시트 한 줄 전체) 안에서 center_x 에 가장 가까운 물체 하나를 잘라 (그림, 배경색)으로 돌려준다.
    칸 경계로 자르면 경계를 살짝 넘은 물체가 잘리고 이웃 물체가 끼어서(2026-11-06 바위 시트), 배경과 다른 열이 이어진 구간으로 물체를 찾는다."""
    from PIL import Image, ImageChops

    background = band.getpixel((2, 2))
    diff = ImageChops.difference(band, Image.new("RGB", band.size, background)).convert("L")
    mask = diff.point(lambda v: 255 if v > 18 else 0)
    columns = [mask.crop((x, 0, x + 1, band.height)).getbbox() is not None for x in range(band.width)]
    candidates = [r for r in runs(columns) if r[1] - r[0] > band.width * 0.05]  # 먼지 · 잡티는 버린다
    if not candidates:
        sys.exit("시트에서 물체를 찾지 못했다")
    x0, x1 = min(candidates, key=lambda r: abs((r[0] + r[1]) / 2 - center_x))
    rows = [mask.crop((x0, y, x1, y + 1)).getbbox() is not None for y in range(band.height)]
    y0, y1 = max(runs(rows), key=lambda r: r[1] - r[0])
    return band.crop((x0, y0, x1, y1)), background


def cut_sheet(sheet_path, out_dir):
    """2×2 시트를 네 장으로 자른다 — 순서는 VIEW_KEYS 와 같다"""
    from PIL import Image

    os.makedirs(out_dir, exist_ok=True)
    sheet = Image.open(sheet_path).convert("RGB")
    half_w, half_h = sheet.width // 2, sheet.height // 2
    objects = []
    for index in range(len(VIEW_KEYS)):
        column, row = index % 2, index // 2
        band = sheet.crop((0, row * half_h, sheet.width, (row + 1) * half_h))
        objects.append(isolate(band, center_x=column * half_w + half_w / 2))
    # 네 장 모두 같은 캔버스 크기 · 같은 바닥선 — 방향마다 물체 크기 비율이 시트 그대로 남는다.
    side = int(max(max(o.width, o.height) for o, _ in objects) * 1.2)
    paths = []
    for key, (obj, background) in zip(VIEW_KEYS, objects):
        canvas = Image.new("RGB", (side, side), background)
        canvas.paste(obj, ((side - obj.width) // 2, int(side * 0.9) - obj.height))
        path = os.path.join(out_dir, f"{key}.png")
        canvas.save(path)
        paths.append(path)
        print(f"  {key}: 물체 {obj.width}×{obj.height} → {side}×{side}")
    print(f"시트 자름 → {out_dir}")
    return paths


def common(fast):
    return {
        "model": MODEL,
        "texture": True,
        "pbr": True,
        "texture_version": TEXTURE_VERSION,
        "delight": True,
        **quality(fast),
    }


def main(argv):
    fast = "--fast" in argv
    args = [a for a in argv if a != "--fast"]
    if not args or args[0] == "balance":
        balance()
    elif args[0] == "image" and len(args) == 3:
        balance()
        generate("image-to-model", {"input": upload(args[1]), **common(fast)}, args[2])
    elif args[0] == "multiview" and len(args) == 6:
        balance()
        generate("multiview-to-model", {"inputs": upload_views(args[1:5]), **common(fast)}, args[5])
    elif args[0] == "sheet" and len(args) == 3:
        balance()
        views = cut_sheet(args[1], os.path.join(args[2], "views"))
        generate("multiview-to-model", {"inputs": upload_views(views), **common(fast)}, args[2])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")  # -I 는 PYTHONIOENCODING 을 무시한다 — 윈도 콘솔(CP949)에서 한글이 깨지지 않게
    main(sys.argv[1:])
