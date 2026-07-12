#!/usr/bin/env bash
set -euo pipefail

: "${NOTARY_PROFILE:?Set NOTARY_PROFILE to an existing notarytool keychain profile}"
ARTIFACT="${1:?Usage: scripts/notarize-macos.sh path-to-zip-pkg-or-dmg}"
[[ -f "$ARTIFACT" ]] || { echo "ERROR: missing $ARTIFACT" >&2; exit 1; }

RESULT="$(xcrun notarytool submit "$ARTIFACT" \
  --keychain-profile "$NOTARY_PROFILE" \
  --wait \
  --output-format json)"
echo "$RESULT"

STATUS="$(printf '%s' "$RESULT" | plutil -extract status raw -o - -- -)"
SUBMISSION_ID="$(printf '%s' "$RESULT" | plutil -extract id raw -o - -- -)"

if [[ "$STATUS" != "Accepted" ]]; then
  echo "ERROR: notarization $SUBMISSION_ID finished with status: $STATUS" >&2
  exit 1
fi

case "$ARTIFACT" in
  *.pkg|*.dmg)
    xcrun stapler staple "$ARTIFACT"
    xcrun stapler validate "$ARTIFACT"
    ;;
  *.zip)
    echo "Accepted ZIP submission $SUBMISSION_ID; ZIP containers cannot be stapled."
    ;;
  *)
    echo "ERROR: notarization artifact must be .zip, .pkg, or .dmg" >&2
    exit 1
    ;;
esac
