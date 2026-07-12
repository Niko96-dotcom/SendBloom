#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

normalize() {
  tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]'
}

BANNED_TOKENS=(reverbx rainger igor pamplejuce pamp p001)
REQUIRED_TOKENS=(sendbloom nkmo sblm nikoaudiolabs)

echo "==> Checking required SendBloom metadata..."
cmake_norm="$(normalize < CMakeLists.txt)"
for token in "${REQUIRED_TOKENS[@]}"; do
  if [[ "$cmake_norm" != *"$token"* ]]; then
    echo "ERROR: Required normalized token '$token' not found in CMakeLists.txt" >&2
    exit 1
  fi
done

is_template_token_allowed() {
  local token="$1"
  local relpath="$2"
  [[ "$token" == "pamplejuce" || "$token" == "pamp" || "$token" == "p001" ]] || return 1
  [[ "$relpath" == cmake/* || "$relpath" == cmake-local/* ]]
}

scan_normalized() {
  local normalized="$1"
  local relpath="$2"
  local kind="$3"
  local token

  for token in "${BANNED_TOKENS[@]}"; do
    if is_template_token_allowed "$token" "$relpath"; then
      continue
    fi
    if [[ "$normalized" == *"$token"* ]]; then
      echo "ERROR: Banned normalized token '$token' found in $kind: $relpath" >&2
      exit 1
    fi
  done
}

scan_content() {
  local relpath="$1"
  local content_norm

  # Binary assets are covered by their normalized repo-relative filenames.
  [[ "$relpath" == *.png ]] && return 0

  if [[ "$relpath" == "CMakeLists.txt" ]]; then
    content_norm="$({ while IFS= read -r line; do
      line_norm="$(printf '%s' "$line" | normalize)"
      [[ "$line_norm" == includepamplejuce* ]] && continue
      printf '%s\n' "$line"
    done < "$relpath"; } | normalize)"
  else
    content_norm="$(normalize < "$relpath")"
  fi

  scan_normalized "$content_norm" "$relpath" "content"
}

scan_filename() {
  local relpath="$1"
  local path_norm
  path_norm="$(printf '%s' "$relpath" | normalize)"
  scan_normalized "$path_norm" "$relpath" "filename"
}

echo "==> Scanning product-facing contents and filenames for banned identifiers..."
scan_roots=(source tests README.md CMakeLists.txt cmake cmake-local .github/workflows resources/presets resources/ui)
for root in "${scan_roots[@]}"; do
  [[ -e "$root" ]] || continue
  if [[ -f "$root" ]]; then
    scan_content "$root"
    scan_filename "$root"
    continue
  fi

  while IFS= read -r -d '' file; do
    relpath="${file#./}"
    scan_content "$relpath"
    scan_filename "$relpath"
  done < <(find "$root" -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.md' -o -name '*.yml' -o -name '*.yaml' -o -name '*.sh' -o -name '*.xml' -o -name '*.txt' -o -name '*.cmake' -o -name '*.png' \) -print0)
done

echo "==> Checking third-party license citations..."
for file in docs/THIRD_PARTY_LICENSES.md README.md; do
  [[ -f "$file" ]] || { echo "ERROR: Missing $file" >&2; exit 1; }
  grep -qi "r8brain" "$file" || { echo "ERROR: r8brain not cited in $file" >&2; exit 1; }
  grep -qi "MIT" "$file" || { echo "ERROR: MIT license not cited in $file" >&2; exit 1; }
done

echo "Legal metadata audit passed."
