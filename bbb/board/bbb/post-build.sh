#!/bin/sh
# $1 (TARGET_DIR) es el primer argumento que Buildroot pasa automáticamente al script

set -eu

# 1. Call the stock BeagleBone script first to do standard setup
board/beagleboard/beaglebone/post-build.sh "$@"

# 2. Modify the sshd_config file in the final filesystem before packaging
echo "=== Executing Post-Build Script to allow SSH Root ==="
if [ -f "${1}/etc/ssh/sshd_config" ]; then
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' "${1}/etc/ssh/sshd_config"
    sed -i 's/PermitRootLogin.*/PermitRootLogin yes/' "${1}/etc/ssh/sshd_config"
    echo "Configuración de OpenSSH actualizada."
fi
