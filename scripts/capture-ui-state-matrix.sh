#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <EditorSnapshot> [output-directory]" >&2
  exit 2
fi

snapshot=$1
output_dir=${2:-artifacts/ui-state-matrix}

if [[ ! -x "$snapshot" ]]; then
  echo "EditorSnapshot is not executable: $snapshot" >&2
  exit 2
fi

mkdir -p "$output_dir"

states=(default dark gate-pre send clip advanced bypass)
flags=("" "--dark" "--gate-pre" "--send" "--clip" "--advanced" "--bypass")

dimensions() {
  local image=$1
  local width height
  width=$(sips -g pixelWidth "$image" | awk '/pixelWidth:/ { print $2 }')
  height=$(sips -g pixelHeight "$image" | awk '/pixelHeight:/ { print $2 }')
  printf '%sx%s' "$width" "$height"
}

for scale in 1 2; do
  expected="$((420 * scale))x$((780 * scale))"
  for index in "${!states[@]}"; do
    state=${states[$index]}
    output="$output_dir/${state}-${scale}x.png"
    command=("$snapshot" "$output" --scale "$scale")
    if [[ -n "${flags[$index]}" ]]; then
      command+=("${flags[$index]}")
    fi
    "${command[@]}"

    actual=$(dimensions "$output")
    if [[ "$actual" != "$expected" ]]; then
      echo "$output: expected $expected, got $actual" >&2
      exit 1
    fi
  done
done

echo "captured ${#states[@]} states at 1x and 2x in $output_dir"
