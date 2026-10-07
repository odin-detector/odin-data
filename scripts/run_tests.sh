#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "${script_dir}/.." && pwd)"
build_dir="${BUILD_DIR:-${repo_dir}/vscode_build}"
bin_dir="${TEST_BIN_DIR:-${build_dir}/bin}"
lib_dir="${TEST_LIB_DIR:-${build_dir}/lib}"
config_source_dir="${INTEGRATION_CONFIG_DIR:-${repo_dir}/cpp/test/integrationTest/config}"
test_timeout_seconds="${TEST_TIMEOUT_SECONDS:-120}"

if [[ ! "${test_timeout_seconds}" =~ ^[1-9][0-9]*$ ]]; then
  echo "TEST_TIMEOUT_SECONDS must be a positive integer: ${test_timeout_seconds}" >&2
  exit 1
fi

if [[ ! -d "${bin_dir}" ]]; then
  echo "Test binary directory does not exist: ${bin_dir}" >&2
  exit 1
fi

receiver_tests=()
while IFS= read -r test_path; do
  receiver_tests+=("${test_path}")
done < <(find "${bin_dir}" -maxdepth 1 -type f -perm -111 -name 'unit_fr_*' | sort)

processor_tests=()
while IFS= read -r test_path; do
  processor_tests+=("${test_path}")
done < <(find "${bin_dir}" -maxdepth 1 -type f -perm -111 -name 'unit_fp_*' | sort)

if [[ ${#receiver_tests[@]} -eq 0 ]]; then
  echo "No frame-receiver unit tests were found in ${bin_dir}" >&2
  exit 1
fi

if [[ ${#processor_tests[@]} -eq 0 ]]; then
  echo "No frame-processor unit tests were found in ${bin_dir}" >&2
  exit 1
fi

log_dir="$(mktemp -d "${TMPDIR:-/tmp}/odin-tests.XXXXXX")"
trap 'rm -rf "${log_dir}"' EXIT

failed_tests=()
failed_statuses=()
failed_logs=()
test_count=0

run_test()
{
  local test_name="$1"
  shift

  local test_log="${log_dir}/${test_name}.log"
  local test_status

  test_count=$((test_count + 1))
  echo "==> Running ${test_name}"

  if timeout --kill-after=5s "${test_timeout_seconds}s" "$@" 2>&1 | tee "${test_log}"; then
    echo "    PASS"
  else
    test_status=$?
    failed_tests+=("${test_name}")
    failed_statuses+=("${test_status}")
    failed_logs+=("${test_log}")
    if [[ ${test_status} -eq 124 ]]; then
      echo "    TIMEOUT after ${test_timeout_seconds}s"
    else
      echo "    FAIL (exit ${test_status})"
    fi
  fi
}

echo "Frame receiver unit tests"
for test_path in "${receiver_tests[@]}"; do
  run_test "$(basename "${test_path}")" "${test_path}" --log_level=test_suite
done

echo
echo "Frame processor unit tests"
for test_path in "${processor_tests[@]}"; do
  run_test "$(basename "${test_path}")" "${test_path}" --log_level=test_suite
done

echo
echo "Odin-data frame integration test"

required_integration_files=(
  "${bin_dir}/odinDataTest"
  "${bin_dir}/frameReceiver"
  "${bin_dir}/frameProcessor"
  "${bin_dir}/frameSimulator"
  "${bin_dir}/frameTests"
  "${config_source_dir}/dummyUDP.json"
  "${config_source_dir}/dummyUDP-fr.json"
  "${config_source_dir}/dummyUDP-fp.json"
  "${config_source_dir}/testUDP.json"
)

for required_file in "${required_integration_files[@]}"; do
  if [[ ! -e "${required_file}" ]]; then
    echo "Required integration-test file does not exist: ${required_file}" >&2
    exit 1
  fi
done

# Replace the install-prefix placeholder in temporary config copies so every
# process uses the binaries and libraries from vscode_build.
runtime_dir="${log_dir}/runtime"
mkdir -p "${runtime_dir}/test_config"
ln -s "${bin_dir}" "${runtime_dir}/bin"
ln -s "${lib_dir}" "${runtime_dir}/lib"

for config_name in dummyUDP.json dummyUDP-fr.json dummyUDP-fp.json testUDP.json; do
  sed "s|\\\${CMAKE_INSTALL_PREFIX}|${runtime_dir}|g" \
    "${config_source_dir}/${config_name}" >"${runtime_dir}/test_config/${config_name}"
done

run_test \
  "odinDataTest" \
  "${bin_dir}/odinDataTest" \
  "--json=${runtime_dir}/test_config/dummyUDP.json"

if [[ ${#failed_tests[@]} -ne 0 ]]; then
  echo
  echo "${#failed_tests[@]} of ${test_count} test executables failed:"

  for ((index = 0; index < ${#failed_tests[@]}; index++)); do
    echo
    echo "===== ${failed_tests[index]} (exit ${failed_statuses[index]}) ====="
    cat "${failed_logs[index]}"
  done

  exit 1
fi

echo
echo "All ${test_count} test executables passed."
