#!/usr/bin/env bash
# Rasterizes this folder's icon sources into the ones the plugin ships.
#
# The hosts' Qt carries no SVG image plugin, so the module can only load raster
# icons; the sources stay here so a glyph can be edited and re-rendered rather
# than redrawn. 64px covers a 32px control at 2x. The alpha carries the shape
# and the strokes are white: the design system tints a glyph by scaling its
# luminance, so a black one stays black whatever colour it is given. The avatar
# tile draws the group glyph straight on a light gradient, from a black copy.
#
# Usage: tools/icons/make-icons.sh
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
out="$here/../../src/qml/ChatUi/icons"
mkdir -p "$out"

for svg in "$here"/*.svg; do
    name="$(basename "$svg" .svg)"
    magick -background none "$svg" -resize 64x64 -channel RGB -negate +channel "$out/$name.png"
    echo "$name.png"
done

magick -background none "$here/group.svg" -resize 64x64 "$out/group-ink.png"
echo "group-ink.png"
