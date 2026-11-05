#!/usr/bin/env bash
# run_batch.sh <work_dir> <동시 수> <job> [job …] — run_codex_job.sh 를 여러 개 동시에. 이미 raw/<job>.png 가 있으면 건너뛴다.
# 참고 그림은 prompts/<job>.refs(한 줄, 쉼표로 여럿)에 둔다.
set -uo pipefail
WD=$(cd "$1" && pwd); N=$2; shift 2
HERE=$(cd "$(dirname "$0")" && pwd)
for JOB in "$@"; do
  if [[ -f "$WD/raw/$JOB.png" ]]; then echo "$JOB skip (있음)" >&2; continue; fi
  REFS=""; [[ -f "$WD/prompts/$JOB.refs" ]] && REFS=$(tr -d '\r\n' < "$WD/prompts/$JOB.refs")
  printf '%s\t%s\n' "$JOB" "$REFS"
done | xargs -P "$N" -d '\n' -I{} bash -c 'IFS=$'"'"'\t'"'"' read -r JOB REFS <<< "{}"; bash "'"$HERE"'/run_codex_job.sh" "'"$WD"'" "$JOB" "$REFS"'
