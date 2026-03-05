#!/usr/bin/env bash

set -e

# Script to run clang-format on individual files or subdirectories. Finds .cpp/.h
# files recursively but is careful not to operate on files outside the git
# repository or ignored files. Will work on untracked files so they can be
# formatted before commit.

function usage() {
    echo >&2 "Usage: $0 <clang-format options> [files] [directories] ..."
    echo >&2 "  Any clang-format options can be added."
    echo >&2 "  To give a check that formatting is not required use both of the following:"
    echo >&2 "   --dry-run makes no changes to the files"
    echo >&2 "   --Werror gives exit code error if changes are required"
    exit 2
}

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
        --dry-run) dryrun=1; args="$args $1"; shift;;
        --verbose) verbose=1; shift;;
        --*) args="$args $1"; shift;;
        *) pathlist+=($1); shift;;
    esac
done

if [ ${#pathlist[@]} -eq 0 ]; then
    echo >&2 "No files or directories given"
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
        echo ${clang_major_version}
        if [ ${clang_major_version} -ge $required ]; then
            echo "${clang_format} version ${clang_major_version}, required $required"
            break
        fi
        clang_format="notset"
    fi
done
if [ "$clang_format" == "notset" ]; then
    echo >&2 "clang-format version $required not found in \"${bin_names[@]}\""
    exit 3
fi

echo "Using $(${clang_format} --version)"

# Get the prefix and run from the top level
prefix=`git rev-parse --show-prefix`
cd `git rev-parse --show-toplevel`

for path in "${pathlist[@]}"; do
    if [ ! -e $prefix$path ]; then
        echo "$prefix$path does not exist"
        exit 2
    fi
    flist=$(find $prefix$path -name "*.cpp" -o -name "*.h")
    if [ "$flist" == "" ]; then
        echo "Path $path contains no files to process."
        exit 1
    fi

    if [ $verbose -eq 1 ]; then
        for f in $flist; do
            echo "Processing $f"
        done
    fi
    if ! echo $flist | xargs -n 20 -P 20 git ls-files -c -m -o --error-unmatch >/dev/null ; then
        echo "Giving up. I don't format files outside of this repository."
        exit 1
    fi
done

rc=0
for path in "${pathlist[@]}"; do
    if [ $dryrun -eq 0 ]; then
        if find $prefix$path -name "*.cpp" -o -name "*.h" | xargs -n 20 -P 20 ${clang_format} -i $args; then
            echo "Format \"$prefix$path\" succeeded"
        else
            rc=$?
            echo "Format \"$prefix$path\" failed $rc"
        fi
    else
        if find $prefix$path -name "*.cpp" -o -name "*.h" | xargs -n 20 -P 20 ${clang_format} -i $args >& /tmp/format_source_dryrun.log; then
            echo "Format \"$prefix$path\" succeeded"
        else
            rc=$?
            echo "Format \"$prefix$path\" failed $rc"
        fi
        cat /tmp/format_source_dryrun.log

        # Record the approximate number of changes required
        changes=$(grep "Wclang-format-violations" /tmp/format_source_dryrun.log | wc -l)
        if [ $changes -eq 0 ]; then
            echo "No changes required"
        else
            echo "Approximate number of changes required $changes"
        fi
    fi
done

exit $rc
