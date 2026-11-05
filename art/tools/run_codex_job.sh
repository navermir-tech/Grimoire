#!/usr/bin/env bash
# run_codex_job.sh <work_dir> <job> [ref.png,ref2.png]
# 그림 하나 = 새 codex exec 세션 하나. 프롬프트는 prompts/<job>.txt 를 표준 입력으로 넘긴다
# ($imagegen 이 셸에서 펼쳐지지 않고, -i 가 프롬프트를 삼키지 않게). 결과는 raw/<job>.png.
set -uo pipefail
WD=$(cd "$1" && pwd)
JOB=$2
IMGS=${3:-}
cd "$WD"
mkdir -p logs raw
ARGS=(exec --skip-git-repo-check -C "$WD" -s workspace-write
      -c 'model_reasoning_effort="medium"'
      -o "logs/$JOB.last.txt")
if [[ -n "$IMGS" ]]; then
  ARGS+=(-i "$IMGS")
fi
start=$(date +%s)
codex "${ARGS[@]}" < "prompts/$JOB.txt" > "logs/$JOB.log" 2>&1
code=$?
end=$(date +%s)
# codex 가 그림을 만들고 raw/ 로 못 옮길 때(윈도 명령 길이 제한)가 있다 — 이 작업의 세션 폴더에서 직접 가져온다.
# 「가장 최근 그림」 을 집으면 동시에 돌린 다른 작업 그림이 섞인다(2026-11-05 분탑) — 세션 id 로만 찾는다.
if [[ ! -f "raw/$JOB.png" ]]; then
  SESSION=$(grep -m1 -o 'session id: [0-9a-f-]*' "logs/$JOB.log" | awk '{print $3}')
  LAST=$(ls -t "$HOME/.codex/generated_images/$SESSION/"*.png 2>/dev/null | head -1)
  if [[ -n "$SESSION" && -n "$LAST" ]]; then cp "$LAST" "raw/$JOB.png"; fi
fi
if [[ -f "raw/$JOB.png" ]]; then out="raw/$JOB.png"; else out="MISSING"; fi
echo "$JOB exit=$code secs=$((end - start)) output=$out"
