#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# RODIN: build-time assertion that the blob drop list stays disjoint from
# the built-in module roster.  kernel/module/main.c rodin_skip_reason()
# answers a load with success when the requested name either is provided by
# this image (.modinfo roster) or is policy-dropped; the two sets must
# therefore never overlap -- a dropped name that is also built in would
# mislabel an answered load and rot the roster's meaning.  Run from
# scripts/Makefile.vmlinux right after the .modinfo harvest.
#
# usage: rodin_skip_disjoint_check.sh <modinfo-blob> <auto.conf>

MODINFO="$1"
AUTOCONF="$2"

die() {
	echo "RODINCHK: ERROR: $*" >&2
	exit 1
}

[ -r "$MODINFO" ] || die "harvested .modinfo blob not readable: $MODINFO"
[ -r "$AUTOCONF" ] || die "auto.conf not readable: $AUTOCONF"

enabled() {
	sed -n "s/^${1}=y\$/y/p" "$AUTOCONF" | grep -qx y
}

# Nothing to assert without the drop gate.
enabled CONFIG_RODIN_BLOB_DROP || exit 0

# auto.conf stores string values unquoted, .config quoted - accept both.
DROP_RAW=$(sed -n 's/^CONFIG_RODIN_BLOB_DROP_LIST=//p' "$AUTOCONF" \
	| sed 's/^"//; s/"$//')
[ -n "$DROP_RAW" ] || die "CONFIG_RODIN_BLOB_DROP=y but CONFIG_RODIN_BLOB_DROP_LIST is empty"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Roster: "modname.field=value" entries; composite "a:b" covers both names.
# Keep kbuild letter case, treat '-' and '_' as equivalent (rodin_name_eq()).
tr '\0' '\n' < "$MODINFO" \
	| awk -F'.' 'NF > 1 && $1 ~ /^[A-Za-z0-9_:]+$/ {print $1}' \
	| tr ':' '\n' | sed '/^$/d' | tr '-' '_' | sort -u > "$WORK/roster"
[ -s "$WORK/roster" ] || die "roster parse yielded zero module names (broken harvest or filter)"

printf '%s\n' "$DROP_RAW" | tr ' ,' '\n\n' | sed '/^$/d' | tr '-' '_' \
	| sort -u > "$WORK/drop"
[ -s "$WORK/drop" ] || die "drop list tokenization yielded zero names"

ILL=$(grep -v '^[A-Za-z0-9_]*$' "$WORK/drop" || true)
[ -z "$ILL" ] || die "ill-formed drop list tokens: $ILL"

VIOL=$(comm -12 "$WORK/roster" "$WORK/drop")
[ -z "$VIOL" ] || die "names both built-in and policy-dropped: $(echo "$VIOL" | tr '\n' ' ')"

echo "RODINCHK: $(wc -l < "$WORK/roster") built-in names x $(wc -l < "$WORK/drop") drop tokens, disjoint OK"
exit 0
