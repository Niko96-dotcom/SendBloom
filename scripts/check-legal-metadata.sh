#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BANNED_TERMS=(
  "Rainger"
  "Reverb-X"
  "Igor"
  "Pamplejuce"
  "Pamp"
  "P001"
)

REQUIRED_TERMS=(
  "SendBloom"
  "NkMo"
  "SbLm"
  "Niko Audio Labs"
)

echo "==> Checking required SendBloom metadata..."
for term in "${REQUIRED_TERMS[@]}"; do
  if ! grep -q "$term" CMakeLists.txt; then
    echo "ERROR: Required term '$term' not found in CMakeLists.txt" >&2
    exit 1
  fi
done

echo "==> Scanning product metadata and sources for banned identifiers..."

scan_file() {
  local file="$1"
  for term in "${BANNED_TERMS[@]}"; do
    if grep -qi "$term" "$file"; then
      echo "ERROR: Banned term '$term' found in $file" >&2
      grep -ni "$term" "$file" || true
      exit 1
    fi
  done
}

# CMake product metadata (exclude upstream include(Pamplejuce*) module names)
while IFS= read -r line; do
  [[ "$line" =~ include\(Pamplejuce ]] && continue
  for term in "${BANNED_TERMS[@]}"; do
    if echo "$line" | grep -qi "$term"; then
      echo "ERROR: Banned term '$term' in CMake metadata: $line" >&2
      exit 1
    fi
  done
done < CMakeLists.txt

for path in source tests docs/THIRD_PARTY_LICENSES.md README.md .github/workflows resources/presets resources; do
  [[ -e "$path" ]] || continue
  if [[ -f "$path" ]]; then
    scan_file "$path"
  else
    while IFS= read -r -d '' file; do
      scan_file "$file"
    done < <(find "$path" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.md" -o -name "*.yml" -o -name "*.sh" -o -name "*.xml" -o -name "*.txt" \) -print0)
  fi
done

echo "==> Checking third-party license citations..."
for file in docs/THIRD_PARTY_LICENSES.md README.md; do
  [[ -f "$file" ]] || { echo "ERROR: Missing $file" >&2; exit 1; }
  grep -qi "r8brain" "$file" || { echo "ERROR: r8brain not cited in $file" >&2; exit 1; }
  grep -qi "MIT" "$file" || { echo "ERROR: MIT license not cited in $file" >&2; exit 1; }
done

echo "Legal metadata audit passed."
