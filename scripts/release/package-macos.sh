#!/usr/bin/env bash
# Produce the exact artifact a user downloads, in the only order that is safe.
#
#   1. sign the AU and VST3 bundles
#   2. notarize a zip of those bundles, then staple the bundles themselves
#   3. (pkg-in-dmg) build the installer from the STAPLED bundles, sign it with
#      a Developer ID Installer identity, notarize it, staple it, validate it
#   4. build the DMG from artifacts that are already signed, notarized,
#      stapled and validated
#   5. sign the DMG, notarize it, staple it, validate it
#
# Checksums are NOT produced here. They are generated after this script
# finishes, by scripts/release/checksums.sh, so that they can only ever
# describe a finalised artifact.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd codesign hdiutil ditto plutil

RELEASE_DIR="$(dirname "${BASH_SOURCE[0]}")"

step "Package — contract: $ARTIFACT_CONTRACT, mode: $RELEASE_MODE"
info "version:  $RELEASE_VERSION"
info "build id: $BUILD_ID"
info "dist:     $DIST_DIR"

if [[ "$RELEASE_MODE" == "public" ]]; then
  # Fail before doing any work if a required credential is absent.
  require_application_identity
  require_notary_profile
  [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]] && require_installer_identity
  [[ "$SOURCE_DIRTY" == "0" ]] \
    || die "refusing to build a public release from a dirty working tree (commit or stash first)"
else
  unsigned_banner
fi

mkdir -p "$DIST_DIR"

# ---------------------------------------------------------------------------
# 1. Sign the plugin bundles
# ---------------------------------------------------------------------------

bash "$RELEASE_DIR/sign-macos.sh"

# ---------------------------------------------------------------------------
# 2. Notarize the bundles, then staple them
# ---------------------------------------------------------------------------

if [[ "$RELEASE_MODE" == "public" ]]; then
  step "Notarize plugin bundles"
  NOTARY_ZIP="$DIST_DIR/$NOTARIZE_ZIP_NAME"
  rm -f "$NOTARY_ZIP"
  BUNDLE_STAGE="$(mktemp -d "$DIST_DIR/notary-stage.XXXXXX")"
  ditto "$VST3_BUNDLE" "$BUNDLE_STAGE/SendBloom.vst3"
  ditto "$AU_BUNDLE"   "$BUNDLE_STAGE/SendBloom.component"
  (cd "$BUNDLE_STAGE" && ditto -c -k --keepParent --norsrc . "$NOTARY_ZIP")
  rm -rf "$BUNDLE_STAGE"

  bash "$RELEASE_DIR/notarize-macos.sh" "$NOTARY_ZIP" "$VST3_BUNDLE" "$AU_BUNDLE"
  rm -f "$NOTARY_ZIP"
  ok "plugin bundles notarized and stapled"
else
  warn "skipping notarization — RELEASE_MODE=$RELEASE_MODE. These bundles are NOT notarized."
fi

# ---------------------------------------------------------------------------
# 3. Installer package (pkg-in-dmg contract only)
# ---------------------------------------------------------------------------

DMG_PAYLOAD="$(mktemp -d "$DIST_DIR/dmg-stage.XXXXXX")"
cleanup_payload() { rm -rf "$DMG_PAYLOAD"; }
trap cleanup_payload EXIT

if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  require_cmd pkgbuild productbuild
  step "Build installer package"

  PKG_WORK="$(mktemp -d "$DIST_DIR/pkg-work.XXXXXX")"
  mkdir -p "$PKG_WORK/root-vst3" "$PKG_WORK/root-au" "$PKG_WORK/resources"

  # Built from the stapled bundles produced above, never from the raw build.
  ditto "$VST3_BUNDLE" "$PKG_WORK/root-vst3/SendBloom.vst3"
  ditto "$AU_BUNDLE"   "$PKG_WORK/root-au/SendBloom.component"

  pkgbuild --quiet \
    --root "$PKG_WORK/root-vst3" \
    --identifier "$BUNDLE_ID.vst3" \
    --version "$VERSION_CORE" \
    --install-location "$VST3_INSTALL_DIR" \
    "$PKG_WORK/SendBloom-VST3.pkg"

  pkgbuild --quiet \
    --root "$PKG_WORK/root-au" \
    --identifier "$BUNDLE_ID.au" \
    --version "$VERSION_CORE" \
    --install-location "$AU_INSTALL_DIR" \
    "$PKG_WORK/SendBloom-AU.pkg"

  cp "$ROOT/LICENSE" "$PKG_WORK/resources/LICENSE.txt"
  cp "$ROOT/RELEASE_NOTES.md" "$PKG_WORK/resources/ReadMe.txt"

  cat >"$PKG_WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>SendBloom $RELEASE_VERSION</title>
    <organization>com.nikoaudiolabs</organization>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <license file="LICENSE.txt"/>
    <readme file="ReadMe.txt"/>
    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
    </choices-outline>
    <choice id="vst3" title="VST3 plug-in" description="Installs SendBloom.vst3 into $VST3_INSTALL_DIR">
        <pkg-ref id="$BUNDLE_ID.vst3"/>
    </choice>
    <choice id="au" title="Audio Unit" description="Installs SendBloom.component into $AU_INSTALL_DIR">
        <pkg-ref id="$BUNDLE_ID.au"/>
    </choice>
    <pkg-ref id="$BUNDLE_ID.vst3" version="$VERSION_CORE" onConclusion="none">SendBloom-VST3.pkg</pkg-ref>
    <pkg-ref id="$BUNDLE_ID.au" version="$VERSION_CORE" onConclusion="none">SendBloom-AU.pkg</pkg-ref>
</installer-gui-script>
EOF

  productbuild --quiet \
    --distribution "$PKG_WORK/distribution.xml" \
    --package-path "$PKG_WORK" \
    --resources "$PKG_WORK/resources" \
    "$PKG_WORK/SendBloom-unsigned.pkg"
  ok "built component and product packages"

  if [[ "$RELEASE_MODE" == "public" ]]; then
    step "Sign installer package"
    productsign --sign "$DEVELOPER_ID_INSTALLER" \
      "$PKG_WORK/SendBloom-unsigned.pkg" "$PKG_PATH"
    pkgutil --check-signature "$PKG_PATH" | sed 's/^/      /'
    pkgutil --check-signature "$PKG_PATH" | grep -q 'Developer ID Installer' \
      || die "$PKG_PATH is not signed by a Developer ID Installer identity"
    ok "installer signed"

    bash "$RELEASE_DIR/notarize-macos.sh" "$PKG_PATH"

    step "Assess installer with Gatekeeper"
    spctl --assess --type install --verbose=4 "$PKG_PATH" 2>&1 | sed 's/^/      /'
    ok "installer accepted as an install source"
  else
    mv "$PKG_WORK/SendBloom-unsigned.pkg" "$PKG_PATH"
    warn "installer is UNSIGNED and UNNOTARIZED: $PKG_PATH"
  fi

  rm -rf "$PKG_WORK"

  cp "$PKG_PATH" "$DMG_PAYLOAD/$PKG_NAME"
else
  step "Contract bundles-in-dmg — no installer package"
  mkdir -p "$DMG_PAYLOAD/VST3" "$DMG_PAYLOAD/Components"
  ditto "$VST3_BUNDLE" "$DMG_PAYLOAD/VST3/SendBloom.vst3"
  ditto "$AU_BUNDLE"   "$DMG_PAYLOAD/Components/SendBloom.component"
  ok "staged plugin bundles for the disk image"
fi

# ---------------------------------------------------------------------------
# 4. Disk image payload
# ---------------------------------------------------------------------------

step "Stage disk image payload"

cp "$ROOT/LICENSE" "$DMG_PAYLOAD/LICENSE.txt"
cp "$ROOT/docs/THIRD_PARTY_LICENSES.md" "$DMG_PAYLOAD/THIRD_PARTY_LICENSES.txt"
cp "$ROOT/RELEASE_NOTES.md" "$DMG_PAYLOAD/RELEASE_NOTES.md"

if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  cat >"$DMG_PAYLOAD/INSTALL.txt" <<EOF
SendBloom $RELEASE_VERSION — install
=====================================

Open $PKG_NAME and follow the installer.

It installs:
  $VST3_INSTALL_DIR/SendBloom.vst3
  $AU_INSTALL_DIR/SendBloom.component

Verify what you downloaded before installing:
  shasum -a 256 -c SHA256SUMS.txt

Uninstalling, upgrading and troubleshooting: docs/install.md in the
repository, https://github.com/Niko96-dotcom/SendBloom

Build identity: $BUILD_ID
EOF
else
  cat >"$DMG_PAYLOAD/INSTALL.txt" <<EOF
SendBloom $RELEASE_VERSION — install
=====================================

Copy the plug-ins into the system plug-in folders:

  VST3/SendBloom.vst3          ->  $VST3_INSTALL_DIR/
  Components/SendBloom.component ->  $AU_INSTALL_DIR/

From a terminal:
  sudo ditto "VST3/SendBloom.vst3" "$VST3_INSTALL_DIR/SendBloom.vst3"
  sudo ditto "Components/SendBloom.component" "$AU_INSTALL_DIR/SendBloom.component"

Verify what you downloaded before installing:
  shasum -a 256 -c SHA256SUMS.txt

Uninstalling, upgrading and troubleshooting: docs/install.md in the
repository, https://github.com/Niko96-dotcom/SendBloom

Build identity: $BUILD_ID
EOF
fi

if [[ "$RELEASE_MODE" != "public" ]]; then
  cat >"$DMG_PAYLOAD/DO-NOT-DISTRIBUTE.txt" <<EOF
This disk image was produced by a LOCAL REHEARSAL run
(RELEASE_MODE=local-unsigned).

It is ad-hoc signed, NOT notarized and NOT stapled. It is not a release,
must not be published or shared, and will be refused by Gatekeeper on any
machine other than the one that built it.

Build identity: $BUILD_ID
EOF
fi
ok "payload staged"

# ---------------------------------------------------------------------------
# 5. Build, sign, notarize, staple the disk image
# ---------------------------------------------------------------------------

step "Build disk image"
rm -f "$DMG_PATH"
VOL_NAME="SendBloom $RELEASE_VERSION"
[[ "$RELEASE_MODE" == "public" ]] || VOL_NAME="SendBloom $RELEASE_VERSION LOCAL"

hdiutil create \
  -volname "$VOL_NAME" \
  -srcfolder "$DMG_PAYLOAD" \
  -fs HFS+ \
  -format UDZO \
  -ov \
  -quiet \
  "$DMG_PATH"
[[ -f "$DMG_PATH" ]] || die "hdiutil produced no disk image at $DMG_PATH"
ok "built $(basename "$DMG_PATH")"

if [[ "$RELEASE_MODE" == "public" ]]; then
  step "Sign disk image"
  codesign --force --sign "$DEVELOPER_ID_APPLICATION" --timestamp "$DMG_PATH"
  codesign --verify --strict --verbose=2 "$DMG_PATH" 2>&1 | sed 's/^/      /'
  dmg_info="$(codesign --display --verbose=4 "$DMG_PATH" 2>&1)"
  codesign_is_adhoc "$dmg_info" && die "disk image carries an ad-hoc signature"
  codesign_authority_is_developer_id_application "$dmg_info" \
    || die "disk image is not signed by a Developer ID Application authority"
  codesign_has_secure_timestamp "$dmg_info" \
    || die "disk image was signed without a secure timestamp"
  ok "disk image signed"

  bash "$RELEASE_DIR/notarize-macos.sh" "$DMG_PATH"

  step "Assess disk image with Gatekeeper"
  spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_PATH" 2>&1 | sed 's/^/      /'
  ok "disk image accepted by Gatekeeper"
else
  codesign --force --sign - --timestamp=none "$DMG_PATH"
  warn "disk image is ad-hoc signed, NOT notarized, NOT stapled: $DMG_PATH"
fi

cleanup_payload
trap - EXIT

echo
step "Artifacts"
ls -la "$DIST_DIR" | sed 's/^/      /'
echo
printf '%spackage-macos: PASS%s  primary artifact: %s\n' "$_c_grn" "$_c_off" "$(basename "$DMG_PATH")"
