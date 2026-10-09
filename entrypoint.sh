#!/bin/bash
set -e

cd /home/br-user/bbb-buildroot-project

if [ ! -d ".git" ]; then
    echo "Volumen vacío. Clonando repositorio principal y submódulo Buildroot..."
    git clone --recursive "$REPO_URL" .
    echo "Clonación completada con éxito."
else
    echo "El repositorio ya existe en el volumen. Sincronizando ramas..."
    git fetch origin || echo "Aviso: no se pudo sincronizar origin; continuando con el checkout existente."
fi

git config --global --add safe.directory /home/br-user/bbb-buildroot-project

if [ -f ".pre-commit-config.yaml" ]; then
    pre-commit install
fi

if [ "$1" = "bash" ] && [ ! -t 0 ]; then
    echo "Entorno listo. Esperando conexión del IDE..."
    exec tail -f /dev/null
fi

exec "$@"
