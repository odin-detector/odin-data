#!/usr/bin/env bash

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${BUILD_DIR:-${repo_dir}/vscode_build}"
test_dir="${UNIT_TEST_DIR:-${build_dir}/bin}"

if [[ ! -d "${test_dir}" ]]; then
  echo "Unit-test directory does not exist: ${test_dir}" >&2
  exit 1
fi

tests=()
while IFS= read -r test_path; do
  tests+=("${test_path}")
done < <(
  find "${test_dir}" -type f -perm -111 \
    \( -name 'unit_fr_*' -o -name 'unit_fp_*' \) | sort
)

if [[ ${#tests[@]} -eq 0 ]]; then
  echo "No unit-test executables were found in ${test_dir}" >&2
  exit 1
fi

log_dir="$(mktemp -d "${TMPDIR:-/tmp}/odin-unit-tests.XXXXXX")"
trap 'rm -rf "${log_dir}"' EXIT

failed_tests=()
failed_statuses=()
failed_logs=()

for test_path in "${tests[@]}"; do
  test_name="$(basename "${test_path}")"
  test_log="${log_dir}/${test_name}.log"

  echo "==> Running ${test_name}"
  if "${test_path}" --log_level=test_suite >"${test_log}" 2>&1; then
    echo "    PASS"
  else
    test_status=$?
    failed_tests+=("${test_name}")
    failed_statuses+=("${test_status}")
    failed_logs+=("${test_log}")
    echo "    FAIL"
  fi
done

if [[ ${#failed_tests[@]} -ne 0 ]]; then
  echo
  echo "${#failed_tests[@]} of ${#tests[@]} unit-test executables failed:"

  for ((index = 0; index < ${#failed_tests[@]}; index++)); do
    echo
    echo "===== ${failed_tests[index]} (exit ${failed_statuses[index]}) ====="
    cat "${failed_logs[index]}"
  done

  exit 1
fi

echo "All ${#tests[@]} unit-test executables passed."
