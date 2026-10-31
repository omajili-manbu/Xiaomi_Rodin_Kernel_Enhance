#!/usr/bin/env python3
"""Pin the vendored SukiSU-Ultra version into drivers/kernelsu/Kbuild.

drivers/kernelsu is an in-tree copy of the SukiSU-Ultra kernel module
(anchored by drivers/kernelsu/.sukisu_tag). The upstream Kbuild resolves
its version via git/curl probes; inside a kernel git repository those
probes would report the kernel tree's identity and hit the network on
every build. This script replaces the probe expressions with literals
computed from the released tag, keeping builds hermetic:

    KSU_VERSION = VERSION_BASE(40000) + bare_count - VERSION_OFFSET(2815)

--count must be the bare rev-list count of the release tag (the Kbuild
adds base/offset itself); the resulting formula value equals the number
in the SukiSU manager APK asset name for the same release. Anchors match
the v4.3.0 Kbuild shape; any upstream restructure exits non-zero so the
caller stops instead of shipping a broken pin. The sync workflow re-runs
this after every upstream landing.

rodin 6.18 (2026-10-31): written for the 6.18 in-tree landing; not
derived from any other tree's pin tooling.
"""
import argparse
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument('--kbuild', required=True)
ap.add_argument('--tag', required=True, help='release tag, v prefix optional')
ap.add_argument('--count', required=True, type=int,
                help='bare rev-list count of the release tag')
ap.add_argument('--sha8', required=True)
ap.add_argument('--branch', default='main')
a = ap.parse_args()
a.tag = a.tag[1:] if a.tag.startswith('v') else a.tag
if a.count <= 0:
    sys.exit(f'nonsense count: {a.count}')

text = open(a.kbuild).read()

m_base = re.search(r'^VERSION_BASE[ \t]+:=[ \t]+([0-9]+)[ \t]*$', text, re.M)
m_off = re.search(r'^VERSION_OFFSET[ \t]+:=[ \t]+([0-9]+)[ \t]*$', text, re.M)
if not (m_base and m_off):
    sys.exit('VERSION_BASE/VERSION_OFFSET not found in Kbuild - refusing to pin blindly')
formula = a.count + int(m_base.group(1)) - int(m_off.group(1))

SUBS = [
    (r'^git_short_sha    = \$\(shell .*$',
     f'git_short_sha    = {a.sha8}'),
    (r'^git_branch       = \$\((?:if|shell) .*$',
     f'git_branch       = {a.branch}'),
    (r'^git_commit_count = \$\(shell .*$',
     f'git_commit_count = {a.count}'),
    (r'^git_latest_tag   = \$\(shell .*$',
     f'git_latest_tag   = {a.tag}'),
    (r'^\$\(shell cd \$\(KSU_SRC\) 2>/dev/null && \[ -f \.\./\.git/shallow \].*$',
     '# vendored in-tree: no git query, no network query at build time'),
    (r'^KSU_GITHUB_VER   := \$\(shell .*$',
     f'KSU_GITHUB_VER   := {a.tag}'),
    (r'^GITHUB_COMMITS   := \$\(shell .*$',
     'GITHUB_COMMITS   :='),
]
for pat, repl in SUBS:
    text, n = re.subn(pat, repl, text, flags=re.M)
    if n != 1:
        sys.exit(f'anchor matched {n} times (expected 1): {pat}\n'
                 'Kbuild structure changed - refusing to pin blindly')

marker = (
    f'# Vendored as an in-tree copy of SukiSU-Ultra v{a.tag} @ {a.sha8}\n'
    f'# (release tag v{a.tag}, bare rev-list count {a.count}, formula value {formula}).\n'
    '# The manager asset name carries the formula value\n'
    '# VERSION_BASE + count - VERSION_OFFSET; pair against the formula, not the count.\n'
    '# Do NOT restore the $(shell git/curl) probes below: inside the kernel git\n'
    '# repository they would report the kernel tree identity and hit the network\n'
    '# on every build. The sync workflow re-pins this block on every landing.\n'
)
text = text.replace('GIT_BIN  := ', marker + 'GIT_BIN  := ', 1)

open(a.kbuild, 'w').write(text)
print(f'pinned: v{a.tag} bare count {a.count} -> formula value {formula}, '
      f'sha {a.sha8}, branch {a.branch}')
