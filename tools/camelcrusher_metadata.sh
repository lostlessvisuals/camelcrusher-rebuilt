#!/usr/bin/env bash
set -euo pipefail

camelcrusher_repo_root() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  cd "$script_dir/.." && pwd
}

camelcrusher_metadata_file() {
  printf '%s/cmake/CamelCrusherMetadata.cmake\n' "$(camelcrusher_repo_root)"
}

camelcrusher_metadata_value() {
  local key="$1"
  local metadata_file
  metadata_file="$(camelcrusher_metadata_file)"
  sed -nE "s/^set\\(${key} \"([^\"]*)\"\\)$/\\1/p" "$metadata_file" | tail -n 1
}
