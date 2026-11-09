#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
export_root="${1:-$repo_root/open-source-export}"
export_repo="$export_root/camelcrusher-rebuilt"

if [[ -e "$export_repo" ]]; then
  echo "Refusing to replace an existing export. Choose a new export directory." >&2
  exit 1
fi
mkdir -p "$export_root"

rsync -a \
  --exclude '/build*/' \
  --exclude '.git/' \
  --exclude 'release/' \
  --exclude 'open-source-export/' \
  --exclude 'reference/' \
  --exclude '.DS_Store' \
  --exclude '__pycache__/' \
  --exclude '*.pyc' \
  "$repo_root/" "$export_repo/"

echo "Prepared GitHub export at $export_repo"
