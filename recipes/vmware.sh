#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: Apache-2.0

# recipes/vmware.sh -- package a VMware Workstation machine for the kernel ISO, <build>/x86/<name>-vmware.zip ("phe image
# recipe vmware -b pc [make_vmware.py options]": -n name, -m MB of RAM, -c CPUs, -d MB of the C: disk, -o file). Runs in
# the toolchain container, after "phe image recipe kernel".
set -eu
exec python3 "$(dirname "$0")/../tools/make_vmware.py" "$@"
