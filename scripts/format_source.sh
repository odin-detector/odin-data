#!/usr/bin/env bash

set -e

# Script to run clang-format on individual files or subdirectories. Finds .cpp/.h
# files recursively but is careful not to operate on files outside the git
# repository or ignored files. Will work on untracked files so they can be
# formatted before commit.

normal='\e[0;30m'
red='\e[0;31m'
green='\e[0;32m'

function report() {
    echo -e "[$scriptname] ${green}$@${normal}"
}

function report_err() {
    echo >&2 -e "[$scriptname] ${red}$@${normal}"
}

function usage() {
    report "Usage: $0 <clang-format options> [files] [directories] ..."
    report "  Any clang-format options can be added."
    report "  To give a check that formatting is not required use both of the following:"
    report "   --dry-run makes no changes to the files"
    report "   --Werror gives exit code error if changes are required"
    exit 2
}

scriptname=$(basename $0)

if [ "$#" -eq 0 ]; then
    usage
fi

args=
dryrun=0
pathlist=()
verbose=0
while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h) usage;;
        --dry-run|-n) dryrun=1; args="$args --dry-run"; shift;;
        --verbose) verbose=1; shift;;
        --*) args="$args $1"; shift;;
        *) pathlist+=($1); shift;;
    esac
done

if [ ${#pathlist[@]} -eq 0 ]; then
    report_err "No files or directories given"
    exit 2
fi

# We need clang-format at least this version
required=20
bin_names=("clang-format" "clang-format-20")
clang_format="notset"
for bin in ${bin_names[@]}; do
    if which $bin &> /dev/null; then
        clang_format=$bin
        clang_major_version=$(${clang_format} --version | grep -Po "(?<=version )[^ ]+" | cut -d "." -f 1)
        if [ ${clang_major_version} -ge $required ]; then
            break
        fi
        clang_format="notset"
    fi
done
if [ "$clang_format" == "notset" ]; then
    report_err "clang-format version $required not found in \"${bin_names[@]}\""
    exit 3
fi

report "Using $(${clang_format} --version)"

# Get the prefix and run from the top level
prefix=`git rev-parse --show-prefix`
cd `git rev-parse --show-toplevel`

for path in "${pathlist[@]}"; do
    if [ ! -e $prefix$path ]; then
        report_err "$prefix$path does not exist"
        exit 2
    fi
    flist=$(find $prefix$path -name "*.cpp" -o -name "*.h")
    if [ "$flist" == "" ]; then
        report_err "Path $path contains no files to process."
        exit 1
    fi

    if [ $verbose -eq 1 ]; then
        for f in $flist; do
            report "Processing $f"
        done
    fi
    if ! echo $flist | xargs -n 20 -P 20 git ls-files -c -m -o --error-unmatch >/dev/null ; then
        report_err "Giving up. I don't format files outside of this repository."
        exit 1
    fi
done

rc=0
for path in "${pathlist[@]}"; do
    # Use dry-run to determine how many changes are required, if any. This should be fast enough
    # to always run clang-format twice.
    if find $prefix$path -name "*.cpp" -o -name "*.h" | xargs -n 20 -P 20 ${clang_format} --dry-run --Werror >& /tmp/format_source_dryrun.log; then
        report "\"$prefix$path\": Passed clang-format check. No formatting changes required"
    else
        check_rc=$?
        # Record the approximate number of changes required
        changes=$(grep "Wclang-format-violations" /tmp/format_source_dryrun.log | wc -l)
        if [ $dryrun -eq 1 ]; then
            rc=${check_rc}
            if [ $changes -le 30 ]; then
                cat /tmp/format_source_dryrun.log
            fi

            report_err "\"$prefix$path\": Failed clang-format check. Approximately $changes changes required"
            if [ $changes -gt 30 ]; then
                mv /tmp/format_source_dryrun.log /tmp/format_source_dryrun_${path}.log
                report_err "    Required changes shown in \"/tmp/format_source_dryrun_${path}.log\""
            fi
        else
            report "\"$prefix$path\": Approximately $changes clang-format changes required"
            # Formatting is required, so actually run the formatter
            if find $prefix$path -name "*.cpp" -o -name "*.h" | xargs -n 20 -P 20 ${clang_format} -i $args; then
                report "   Formatting succeeded"
            else
                rc=$?
                report_err "   Error: Formatting failed with status $rc"
            fi
        fi
    fi
done

exit $rc
