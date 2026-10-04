#!/bin/sh

# El primer argumento ($1) será la ruta de imágenes, el segundo ($2) el nombre de la placa
BOARD_NAME="$2"
BOARD_DIR="${BR2_EXTERNAL_BBB_PATH}/board/${BOARD_NAME}"

echo "-> Generando imagen final para el host/arquitectura: ${BOARD_NAME}"

# 1. Copiar los archivos de arranque específicos (si existen) a la carpeta temporal de binarios
if [ -d "${BOARD_DIR}" ]; then
    find "${BOARD_DIR}" -type f ! -name "genimage.cfg" -exec cp {} "${BINARIES_DIR}/" \;
fi

# 2. Ejecutar genimage apuntando al archivo de configuración de esa placa concreta
support/scripts/genimage.sh -c "${BOARD_DIR}/genimage.cfg"

