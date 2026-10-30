#!/bin/sh
set -eu

# $1 (TARGET_DIR) is the first argument that Buildroot automatically passes to the script
TARGET_DIR=$1

# 1. Call the stock BeagleBone script first to do standard setup
board/beagleboard/beaglebone/post-build.sh "$@"

# 2. Create the configuration directory if it does not exist
mkdir -p "${TARGET_DIR}/etc/sudoers.d"

# 3. Allow the 'wheel' group to run sudo by entering their own password
echo "%wheel ALL=(ALL:ALL) ALL" > "${TARGET_DIR}/etc/sudoers.d/wheel"

# 4. Apply the strict permissions required by sudo (0440)
chmod 0440 "${TARGET_DIR}/etc/sudoers.d/wheel"
