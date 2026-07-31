#!/usr/bin/env bash
# Release gate: ensure CHANGELOG.md covers every commit since the previous tag.
# Used by release.yml (changelog-check job) when a v* tag is pushed.
# Usage: bash scripts/changelog-check.sh [CURRENT_TAG]
#   CURRENT_TAG from arg or GITHUB_REF_NAME env var.
set -euo pipefail

CURRENT_TAG="${1:-${GITHUB_REF_NAME:-}}"
[ -z "$CURRENT_TAG" ] && { echo "Error: current tag required (arg or GITHUB_REF_NAME)" >&2; exit 1; }

PREV_TAG=$(git tag --sort=-version:refname | grep -v "^${CURRENT_TAG}$" | head -1)
[ -z "$PREV_TAG" ] && { echo "No previous tag found — skipping check"; exit 0; }
echo "Checking CHANGELOG coverage: ${PREV_TAG}..${CURRENT_TAG}"

python3 - "$PREV_TAG" "$CURRENT_TAG" <<'PY'
import re, subprocess, sys
prev, cur = sys.argv[1], sys.argv[2]
changelog = open('CHANGELOG.md', encoding='utf-8').read().lower()
log = subprocess.run(['git', 'log', f'{prev}..{cur}', '--format=%H|%P|%s'],
                     capture_output=True, text=True).stdout.splitlines()

STOP = {'the','and','for','with','from','into','was','are','not','all','add','fix','use','via',
        'new','out','its','this','that','code','docs','ci','pr','chore','refactor','merge','bump',
        'update','remove','replace','retired','current','release','clean','branch','main',
        'workflow','build','stage','step'}

def covered(subject):
    """Return None if covered, else a reason string."""
    prs = re.findall(r'\(#(\d+)\)', subject)
    if prs:
        for p in prs:
            if f'#{p}' not in changelog:
                return f"PR #{p} not mentioned in CHANGELOG.md"
        return None
    # direct commit (no PR number): match keywords, docs: commits are self-documenting
    if re.match(r'^docs:', subject):
        return None
    core = re.sub(r'^(fix|feat|docs|chore|ci|refactor|test|build|perf|style)(\([^)]*\))?:\s*', '', subject)
    words = [w for w in re.split(r'[^a-z0-9]+', core.lower()) if len(w) > 3 and w not in STOP]
    if not words:
        return None  # nothing meaningful to match
    if any(w in changelog for w in words):
        return None
    return f"no keyword {words[:4]} found in CHANGELOG.md"

missing = []
for line in log:
    sha, parents, subject = line.split('|', 2)
    if len(parents.split()) >= 2:
        continue  # merge commit — not a feature change, no changelog entry required
    reason = covered(subject)
    if reason:
        missing.append(f"  {sha[:7]} {subject}\n      -> {reason}")

if missing:
    print(f"FAIL: {len(missing)}/{len(log)} commit(s) not covered by CHANGELOG.md (since {prev}):")
    print('\n'.join(missing))
    sys.exit(1)
print(f"PASS: all {len(log)} commit(s) since {prev} are covered by CHANGELOG.md")
PY
