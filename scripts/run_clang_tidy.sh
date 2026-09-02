#!/usr/bin/env bash

set -euo pipefail

# Run a curated set of clang-tidy C++ modernisation checks over translation
# units in a CMake compilation database. Header diagnostics are limited to
# Odin-owned C++ code; vendored RapidJSON and ZeroMQ headers are excluded.

script_name=$(basename "$0")
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd -- "${script_dir}/.." && pwd)

default_checks="-*,\
modernize-avoid-bind,\
modernize-deprecated-headers,\
modernize-loop-convert,\
modernize-make-shared,\
modernize-make-unique,\
modernize-replace-auto-ptr,\
modernize-use-equals-default,\
modernize-use-equals-delete,\
modernize-use-noexcept,\
modernize-use-nullptr,\
modernize-use-override,\
modernize-use-using"

build_dir="${CLANG_TIDY_BUILD_DIR:-${repo_dir}/vscode_build}"
checks="${CLANG_TIDY_CHECKS:-${default_checks}}"
jobs="${CLANG_TIDY_JOBS:-4}"
apply_fixes=0
quiet=1
warnings_as_errors=""
source_paths=()

report()
{
    printf '[%s] %s\n' "${script_name}" "$*"
}

report_error()
{
    printf '[%s] Error: %s\n' "${script_name}" "$*" >&2
}

usage()
{
    cat <<EOF
Usage: ${script_name} [options] [source-file-or-directory ...]

Run clang-tidy against translation units in a CMake compile_commands.json.
Paths are resolved relative to the repository root. With no paths, all C++
translation units under cpp/ are checked. Headers are analysed when included
by a translation unit; vendored RapidJSON and ZeroMQ headers are excluded.

Options:
  -p, --build-dir DIR          CMake build directory (default: vscode_build)
  -j, --jobs N                 Parallel clang-tidy processes (default: 4)
      --checks LIST            Override the curated clang-tidy checks
      --warnings-as-errors LIST
                               Upgrade matching checks to errors
      --fix                    Apply clang-tidy fix-its to source files
      --verbose                Show run-clang-tidy progress output
  -h, --help                   Show this help

Environment overrides:
  CLANG_TIDY_BUILD_DIR, CLANG_TIDY_CHECKS, CLANG_TIDY_JOBS

Examples:
  scripts/${script_name}
  scripts/${script_name} cpp/frameProcessor/src/FileWriterPlugin.cpp
  scripts/${script_name} --jobs 8 cpp/common cpp/frameReceiver
  scripts/${script_name} --checks='-*,modernize-use-nullptr' cpp/common
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--build-dir)
            [[ $# -ge 2 ]] || { report_error "$1 requires a directory"; exit 2; }
            build_dir=$2
            shift 2
            ;;
        --build-dir=*)
            build_dir=${1#*=}
            [[ -n "${build_dir}" ]] || { report_error "--build-dir requires a directory"; exit 2; }
            shift
            ;;
        -j|--jobs)
            [[ $# -ge 2 ]] || { report_error "$1 requires a positive integer"; exit 2; }
            jobs=$2
            shift 2
            ;;
        --jobs=*)
            jobs=${1#*=}
            [[ -n "${jobs}" ]] || { report_error "--jobs requires a positive integer"; exit 2; }
            shift
            ;;
        --checks)
            [[ $# -ge 2 ]] || { report_error "$1 requires a checks list"; exit 2; }
            checks=$2
            shift 2
            ;;
        --checks=*)
            checks=${1#*=}
            [[ -n "${checks}" ]] || { report_error "--checks requires a checks list"; exit 2; }
            shift
            ;;
        --warnings-as-errors)
            [[ $# -ge 2 ]] || { report_error "$1 requires a checks list"; exit 2; }
            warnings_as_errors=$2
            shift 2
            ;;
        --warnings-as-errors=*)
            warnings_as_errors=${1#*=}
            [[ -n "${warnings_as_errors}" ]] || { report_error "--warnings-as-errors requires a checks list"; exit 2; }
            shift
            ;;
        --fix)
            apply_fixes=1
            shift
            ;;
        --verbose)
            quiet=0
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            source_paths+=("$@")
            break
            ;;
        -*)
            report_error "Unknown option: $1"
            usage >&2
            exit 2
            ;;
        *)
            source_paths+=("$1")
            shift
            ;;
    esac
done

if [[ ! "${jobs}" =~ ^[1-9][0-9]*$ ]]; then
    report_error "Job count must be a positive integer: ${jobs}"
    exit 2
fi

if [[ "${build_dir}" != /* ]]; then
    build_dir="${repo_dir}/${build_dir}"
fi

compile_commands="${build_dir}/compile_commands.json"
if [[ ! -f "${compile_commands}" ]]; then
    report_error "Compilation database not found: ${compile_commands}"
    report_error "Configure it with: cmake -S ${repo_dir}/cpp -B ${build_dir} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    exit 2
fi

clang_tidy=""
for candidate in clang-tidy-20 clang-tidy; do
    if command -v "${candidate}" >/dev/null 2>&1; then
        candidate_path=$(command -v "${candidate}")
        candidate_major=$("${candidate_path}" --version \
            | sed -nE 's/.*version ([0-9]+).*/\1/p' \
            | head -n 1)
        if [[ -n "${candidate_major}" && "${candidate_major}" -ge 20 ]]; then
            clang_tidy=${candidate_path}
            break
        fi
    fi
done

if [[ -z "${clang_tidy}" ]]; then
    report_error "clang-tidy version 20 or newer was not found"
    exit 3
fi

run_clang_tidy=""
for candidate in run-clang-tidy-20 run-clang-tidy; do
    if command -v "${candidate}" >/dev/null 2>&1; then
        run_clang_tidy=$(command -v "${candidate}")
        break
    fi
done

if [[ -z "${run_clang_tidy}" ]]; then
    report_error "run-clang-tidy was not found"
    exit 3
fi

if [[ ${#source_paths[@]} -eq 0 ]]; then
    source_paths=("cpp")
fi

regex_escape()
{
    sed 's/[][\\.^$*+?{}()|]/\\&/g' <<<"$1"
}

source_patterns=()
for source_path in "${source_paths[@]}"; do
    if [[ "${source_path}" == /* ]]; then
        absolute_path=${source_path}
    else
        absolute_path="${repo_dir}/${source_path}"
    fi

    if [[ ! -e "${absolute_path}" ]]; then
        report_error "Source path does not exist: ${source_path}"
        exit 2
    fi

    absolute_path=$(cd -- "$(dirname -- "${absolute_path}")" && pwd)/$(basename -- "${absolute_path}")
    case "${absolute_path}" in
        "${repo_dir}"|"${repo_dir}"/*) ;;
        *)
            report_error "Source path is outside the repository: ${source_path}"
            exit 2
            ;;
    esac

    escaped_path=$(regex_escape "${absolute_path}")
    if [[ -d "${absolute_path}" ]]; then
        source_patterns+=("^${escaped_path}/.*\\.(c|cc|cpp|cxx)$")
    else
        case "${absolute_path}" in
            *.c|*.cc|*.cpp|*.cxx) ;;
            *)
                report_error "Expected a C/C++ source file or directory: ${source_path}"
                exit 2
                ;;
        esac
        source_patterns+=("^${escaped_path}$")
    fi
done

escaped_cpp_dir=$(regex_escape "${repo_dir}/cpp")
escaped_common_include_dir=$(regex_escape "${repo_dir}/cpp/common/include")

command=(
    "${run_clang_tidy}"
    -clang-tidy-binary "${clang_tidy}"
    -p "${build_dir}"
    -j "${jobs}"
    "-checks=${checks}"
    -header-filter "^${escaped_cpp_dir}/"
    -exclude-header-filter "^${escaped_common_include_dir}/(rapidjson|zmq)/"
)

if [[ ${quiet} -eq 1 ]]; then
    command+=(-quiet)
fi
if [[ ${apply_fixes} -eq 1 ]]; then
    command+=(-fix)
fi
if [[ -n "${warnings_as_errors}" ]]; then
    command+=("-warnings-as-errors=${warnings_as_errors}")
fi
command+=("${source_patterns[@]}")

report "Using $("${clang_tidy}" --version | head -n 1)"
report "Compilation database: ${compile_commands}"
report "Parallel jobs: ${jobs}"
if [[ ${apply_fixes} -eq 1 ]]; then
    report "Applying clang-tidy fix-its"
else
    report "Report-only mode; no source files will be changed"
fi

exec "${command[@]}"
