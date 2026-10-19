#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

clang_format_sources=()
astyle_sources=()

append_clang_format_source() {
    local file="${1#"$repo_root/"}"
    file="${file#./}"
    if [[ ! -f "$file" ]]; then
        return
    fi

    case "$file" in
        src/lua/*|src/sol/*|src/third-party/*|tests/catch/*|tools/clang-tidy-plugin/test/*)
            return
            ;;
    esac

    if [[ "$file" == src/*/* || "$file" == tests/* || "$file" == tools/format/* || "$file" == tools/clang-tidy-plugin/* ]]; then
        case "$file" in
            *.cpp|*.h|*.hpp)
                clang_format_sources+=( "$file" )
                ;;
        esac
    fi
}

append_astyle_source() {
    local file="${1#"$repo_root/"}"
    file="${file#./}"
    if [[ ! -f "$file" ]]; then
        return
    fi

    if [[ "$file" =~ ^src/[^/]+\.(cpp|h)$ ]]; then
        astyle_sources+=( "$file" )
    fi
}

if (( $# > 0 )); then
    for file in "$@"; do
        append_clang_format_source "$file"
        append_astyle_source "$file"
    done
else
    while IFS= read -r -d '' file; do
        append_clang_format_source "$file"
    done < <(
        find src -mindepth 2 \
            \( -path 'src/lua/*' -o -path 'src/sol/*' -o -path 'src/third-party/*' \) -prune -o \
            -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0
    )

    while IFS= read -r -d '' file; do
        append_clang_format_source "$file"
    done < <(find tests tools/format tools/clang-tidy-plugin -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0)

    while IFS= read -r -d '' file; do
        append_astyle_source "$file"
    done < <(find src -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.h' \) -print0)
fi

if (( ${#clang_format_sources[@]} > 0 )); then
    if ! command -v clang-format >/dev/null 2>&1; then
        echo "error: clang-format executable was not found" >&2
        exit 1
    fi
    clang-format -i "${clang_format_sources[@]}"
elif (( $# == 0 )); then
    echo "warning: no clang-format C++ files available" >&2
fi

if (( ${#astyle_sources[@]} > 0 )); then
    if command -v astyle >/dev/null 2>&1 &&
        [[ "$(astyle --version 2>&1)" == "Artistic Style Version 3.4.7" ]]; then
        if ! command -v python3 >/dev/null 2>&1; then
            echo "error: Python 3 is required to protect C++ raw strings during formatting" >&2
            exit 1
        fi
        python3 tools/r3d_spike/astyle_347.py --native "$(command -v astyle)" "${astyle_sources[@]}"
    elif command -v uvx >/dev/null 2>&1; then
        uvx --from astyle-py==1.1.0 python tools/r3d_spike/astyle_347.py "${astyle_sources[@]}"
    elif command -v python3 >/dev/null 2>&1; then
        python3 tools/r3d_spike/astyle_347.py "${astyle_sources[@]}"
    else
        echo "error: install astyle 3.4.7, or Python 3 with astyle-py==1.1.0 (uvx is supported)" >&2
        exit 1
    fi
elif (( $# == 0 )); then
    echo "warning: no astyle C++ files available" >&2
fi

if (( ${#clang_format_sources[@]} == 0 && ${#astyle_sources[@]} == 0 && $# > 0 )); then
    echo "warning: no supported C++ files were provided" >&2
fi
