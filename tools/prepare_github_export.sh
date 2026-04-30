#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
export_root="${1:-$repo_root/open-source-export}"
export_repo="$export_root/camelcrusher-rebuilt"

rm -rf "$export_repo"
mkdir -p "$export_root"

rsync -a \
  --exclude 'build/' \
  --exclude 'build-release/' \
  --exclude 'release/' \
  --exclude 'open-source-export/' \
  --exclude 'reference/' \
  --exclude '.DS_Store' \
  --exclude '__pycache__/' \
  --exclude '*.pyc' \
  "$repo_root/" "$export_repo/"

echo "Prepared GitHub export at $export_repo"
