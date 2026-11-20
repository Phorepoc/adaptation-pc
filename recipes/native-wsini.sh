#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

# recipes/native-wsini.sh -- the native-size screen modes ("phe image recipe native-wsini -b pc", in the container; release.sh runs it
# with PHOREPOC_NATIVE=1): links the merged OS tree's board/wsini.txt to os/board/wsini-native.txt (wsini-nga-native.txt with
# NGA=1). In the container, so the link and the merged tree use the container's paths: the release recipe runs on the host,
# where a link to a host path does not resolve in the container and the host's paths.py rebuilds the merged tree for the host.
set -eu
src="$(cd "$(dirname "$0")/.." && pwd)/os/board/wsini-native.txt"
[ "${NGA:-0}" = 1 ] && src="$(cd "$(dirname "$0")/.." && pwd)/os/board/wsini-nga-native.txt"
view="$(python3 "${PHOREPOC_PDK:?run it with phe image recipe native-wsini -b pc}/tools/emitter/paths.py")/board"
[ -f "$src" ] && [ -d "$view" ] || { echo "native-wsini: no $src or no merged board folder $view" >&2; exit 1; }
ln -sfn "$src" "$view/wsini.txt"
echo "native resolution: $view/wsini.txt -> $src"
