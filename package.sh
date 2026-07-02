#!/bin/bash
# Build ONE component-selectable, signed macOS installer (.pkg) for the Pulp
# example plugins: the AU, VST3, and CLAP builds of every example, grouped into
# three user-selectable choices (Audio Units / VST3 / CLAP) in the installer's
# Customize pane.
#
# Unlike the single-product tools/scripts/build_combined_installer.sh (one AU +
# one VST3 + one CLAP), this repo ships EIGHT plugins per format, so each format
# is one choice that installs all of that format's bundles via `pkgbuild --root`.
#
# Each bundle is deep-signed (inner dylibs first so @loader_path stays
# relocatable) before packaging. Notarize the resulting .pkg separately:
#   pulp ship notarize --path <out>/PulpExamplePlugins-<ver>.pkg
#
# Usage:
#   ./package.sh --version 0.1.0 \
#     --sign-identity <Developer ID Application hash> \
#     --installer-identity <Developer ID Installer hash> \
#     [--build DIR] [--out DIR]
set -euo pipefail

VERSION=""; APP_ID=""; INST_ID=""; BUILD="build"; OUT="artifacts"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version) VERSION="$2"; shift 2;;
    --sign-identity) APP_ID="$2"; shift 2;;
    --installer-identity) INST_ID="$2"; shift 2;;
    --build) BUILD="$2"; shift 2;;
    --out) OUT="$2"; shift 2;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done
[[ -n "$VERSION" && -n "$APP_ID" && -n "$INST_ID" ]] || {
  echo "missing required args (--version --sign-identity --installer-identity)" >&2; exit 2; }

STAGE="$(mktemp -d)"; trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$OUT" "$STAGE/comp"
source ~/.config/pulp/secrets/keychain.env 2>/dev/null || true

deep_sign() {  # $1 = bundle
  local b="$1"
  find "$b/Contents/MacOS" -name "*.dylib" -print0 2>/dev/null | while IFS= read -r -d '' d; do
    codesign --force --options runtime --timestamp -s "$APP_ID" "$d"; done
  codesign --force --options runtime --timestamp -s "$APP_ID" "$b"
  codesign --verify --deep --strict "$b"
}

# One selectable component per format; each installs all of that format's bundles.
declare -a K_KIND=(au vst3 clap)
declare -a K_GLOB=("$BUILD/AU/*.component" "$BUILD/VST3/*.vst3" "$BUILD/CLAP/*.clap")
declare -a K_DEST=(/Library/Audio/Plug-Ins/Components /Library/Audio/Plug-Ins/VST3 /Library/Audio/Plug-Ins/CLAP)
declare -a K_TITLE=("Audio Units (AU)" "VST3" "CLAP")
declare -a K_DESC=("Logic Pro, GarageBand, MainStage" "Cubase, Studio One, REAPER, Bitwig" "REAPER, Bitwig, Studio One")

CHOICES=""; DEFS=""; REFS=""
for i in "${!K_KIND[@]}"; do
  kind="${K_KIND[$i]}"; root="$STAGE/root-$kind"; mkdir -p "$root"
  n=0
  for b in ${K_GLOB[$i]}; do
    [[ -e "$b" ]] || continue
    deep_sign "$b"
    cp -R "$b" "$root/"
    n=$((n+1))
  done
  [[ "$n" -gt 0 ]] || { echo "no $kind bundles under ${K_GLOB[$i]}" >&2; exit 2; }
  echo "== $kind: $n bundle(s) =="
  pkg="com.pulp.examples.$kind.pkg"
  pkgbuild --root "$root" --identifier "$pkg" --version "$VERSION" \
    --install-location "${K_DEST[$i]}" "$STAGE/comp/$kind.pkg" >/dev/null
  CHOICES="$CHOICES    <line choice=\"$kind\"/>
"
  DEFS="$DEFS  <choice id=\"$kind\" title=\"${K_TITLE[$i]}\" description=\"${K_DESC[$i]}\"><pkg-ref id=\"$pkg\"/></choice>
"
  REFS="$REFS  <pkg-ref id=\"$pkg\" version=\"$VERSION\">$kind.pkg</pkg-ref>
"
done

cat > "$STAGE/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Pulp Example Plugins</title>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline>
$CHOICES  </choices-outline>
$DEFS$REFS</installer-gui-script>
XML

OUT_PKG="$OUT/PulpExamplePlugins-$VERSION.pkg"
productbuild --distribution "$STAGE/distribution.xml" --package-path "$STAGE/comp" \
  --version "$VERSION" --sign "$INST_ID" "$OUT_PKG"
echo "built: $OUT_PKG"
pkgutil --check-signature "$OUT_PKG" | sed -n '1,4p'
