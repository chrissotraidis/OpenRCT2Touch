#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

failures=0
branch="$(git branch --show-current)"
if [[ -z "$branch" && "${GITHUB_ACTIONS:-}" == "true" ]]; then
    branch="${GITHUB_HEAD_REF:-${GITHUB_REF_NAME:-}}"
fi
# A detached checkout (a release tag, or PadMint building an exact commit) is not a
# branch anyone works on, so only named branches are restricted.
if [[ -n "$branch" && "$branch" != "ipad" && "$branch" != touch/* ]]; then
    printf 'ERROR: unsafe branch %s; use ipad or touch/*\n' "$branch" >&2
    failures=$((failures + 1))
fi

tracked_ref="$(git ls-files ref | grep -v '^ref/README.md$' || true)"
if [[ -n "$tracked_ref" ]]; then
    printf 'ERROR: proprietary ref paths are tracked:\n%s\n' "$tracked_ref" >&2
    failures=$((failures + 1))
fi

unignored_ref="$(git ls-files --others --exclude-standard ref | grep -v '^ref/README.md$' || true)"
if [[ -n "$unignored_ref" ]]; then
    printf 'ERROR: proprietary ref paths are not ignored:\n%s\n' "$unignored_ref" >&2
    failures=$((failures + 1))
fi

upstream_push="$(git remote get-url --push upstream 2>/dev/null || true)"
if [[ -n "$upstream_push" && "$upstream_push" != "DISABLED" ]]; then
    printf 'ERROR: upstream has a usable push URL: %s\n' "$upstream_push" >&2
    failures=$((failures + 1))
fi

if [[ "$failures" -ne 0 ]]; then
    printf 'Repository safety check failed with %d error(s).\n' "$failures" >&2
    exit 1
fi

printf 'Repository safety check passed on %s.\n' "$branch"
