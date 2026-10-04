#!/bin/sh
# $1 (TARGET_DIR) es el primer argumento que Buildroot pasa automáticamente al script

echo "=== Ejecutando Post-Build Script para permitir SSH Root ==="

# Modificar el archivo sshd_config en el sistema de archivos final antes de empaquetar
if [ -f "${1}/etc/ssh/sshd_config" ]; then
    sed -i 's/#PermitRootLogin.*/PermitRootLogin yes/' "${1}/etc/ssh/sshd_config"
    sed -i 's/PermitRootLogin.*/PermitRootLogin yes/' "${1}/etc/ssh/sshd_config"
    echo "Configuración de OpenSSH actualizada."
fi
