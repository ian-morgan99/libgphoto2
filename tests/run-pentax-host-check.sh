#!/usr/bin/env bash
# Hardware-free clean-source build. Never installs packages or touches a camera.
set -euo pipefail
source_dir=$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)
cd "$source_dir"
if [[ $(git branch --show-current) != main || -n $(git status --porcelain) ]]; then
    echo 'STOP: run from clean libgphoto2 main; preserve/review dirty work first.' >&2
    exit 1
fi
for tool in meson ninja sha256sum rg; do
    command -v "$tool" >/dev/null || { echo "Missing installed tool: $tool" >&2; exit 1; }
done
source_sha=$(git rev-parse HEAD)
output_dir=$(mktemp -d /tmp/pentax-host-check.XXXXXXXX)
trap 'result=$?; echo "Host-check exit=$result; artifacts and logs: $output_dir"' EXIT
git rev-parse HEAD | tee "$output_dir/source-commit.txt"
meson setup "$output_dir/build" "$source_dir" \
    --prefix="$output_dir/prefix" --libdir=lib --wrap-mode=nodownload \
    -Dcamlibs=ptp2,directory -Diolibs=libusb1,disk -Ddocs=false \
    -Dpentax_research_capture=false 2>&1 | tee "$output_dir/configure.log"
ninja -C "$output_dir/build" -j4 2>&1 | tee "$output_dir/build.log"
# no-ci contains manual device tests and MUST remain excluded.
meson test -C "$output_dir/build" --no-rebuild --no-suite no-ci \
    --print-errorlogs 2>&1 | tee "$output_dir/tests.log"
if [[ $(git rev-parse HEAD) != "$source_sha" || -n $(git status --porcelain) ]]; then
    echo 'STOP: source changed during build; artifact is not a clean-SHA result.' >&2
    exit 1
fi
meson install -C "$output_dir/build" --no-rebuild 2>&1 | tee "$output_dir/install.log"
rg --files --hidden --no-ignore -0 "$output_dir/prefix/lib" |
    sort -z | xargs -0 sha256sum | tee "$output_dir/installed-sha256.txt"
echo 'PASS: isolated host build/tests/install only. NOT firmware or hardware qualification.'
