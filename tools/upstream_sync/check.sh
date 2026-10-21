#!/usr/bin/env bash
# Keeps upstream merges to game logic only. See docs/en/dev/guides/upstream_sync.md.
#   check.sh report [upstream-ref]   classify the commits upstream has that we do not
#   check.sh guard  [upstream-ref]   after a merge, fail if it changed frontend paths
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"
mode="${1:-report}"
upstream="${2:-upstream/main}"
paths_file="tools/upstream_sync/renderer_paths.txt"

mapfile -t specs < <(sed -e 's/#.*//' -e 's/[[:space:]]*$//' "$paths_file" | grep -v '^$')
pathspecs=()
for spec in "${specs[@]}"; do pathspecs+=( ":(glob)$spec" ); done

case "$mode" in
report)
    git rev-parse --verify -q "$upstream" >/dev/null || { echo "no ref $upstream; run: git fetch upstream" >&2; exit 2; }
    logic=0; frontend=0
    while read -r sha; do
        subject="$(git log -1 --format=%s "$sha")"
        touched="$(git diff-tree --no-commit-id --name-only -r "$sha" -- "${pathspecs[@]}")"
        if [[ -z "$touched" ]]; then
            logic=$((logic + 1)); echo "logic     ${sha:0:10} $subject"
        else
            frontend=$((frontend + 1)); echo "FRONTEND  ${sha:0:10} $subject"
            sed 's/^/            /' <<<"$touched"
        fi
    done < <(git rev-list --reverse --no-merges "HEAD..$upstream")
    echo "-- $logic logic-only, $frontend touching frontend paths"
    echo "-- files upstream changed that the fork also modified (conflict risk):"
    comm -12 <(git diff --name-only "HEAD...$upstream" | sort) \
             <(git diff --name-only --diff-filter=M "$upstream...HEAD" | sort) | sed 's/^/     /'
    ;;
guard)
    # Run on the merge commit (HEAD): compare it with its first parent (our side).
    git rev-parse --verify -q HEAD^2 >/dev/null || { echo "HEAD is not a merge commit" >&2; exit 2; }
    bad="$(git diff --name-only HEAD^1 HEAD -- "${pathspecs[@]}")"
    if [[ -n "$bad" ]]; then
        echo "merge changed frontend paths (restore them: git checkout HEAD^1 -- <path>):" >&2
        sed 's/^/  /' <<<"$bad" >&2
        exit 1
    fi
    echo "ok: merge changed no frontend paths"
    ;;
*)
    echo "usage: $0 report|guard [upstream-ref]" >&2; exit 2 ;;
esac
