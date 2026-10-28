#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

# recipes/vbox.sh -- package the kernel ISO as a VirtualBox appliance, <build>/x86/<name>.ova ("ph image recipe vbox
# -b pc [make_ova.py options]": -n name, -m MB of RAM, -c CPUs, -d MB of the C: disk, -o file). Runs in the toolchain
# container, after "ph image recipe kernel".
set -eu
exec python3 "$(dirname "$0")/../tools/make_ova.py" "$@"
