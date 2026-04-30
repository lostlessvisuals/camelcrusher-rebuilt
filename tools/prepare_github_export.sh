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
  --exclude '.DS_Store' \
  --exclude '__pycache__/' \
  --exclude '*.pyc' \
  --exclude 'reference/original-distribution/' \
  "$repo_root/" "$export_repo/"

mkdir -p "$export_repo/reference/original-distribution"
cat >"$export_repo/reference/original-distribution/README.md" <<'EOF'
# Original Distribution Placeholder

The local development workspace keeps the original CamelCrusher distribution here for reverse-engineering and compatibility research.

It is intentionally excluded from the GitHub export so the public tree stays close to local development without republishing the original vendor distribution.
EOF

echo "Prepared GitHub export at $export_repo"
