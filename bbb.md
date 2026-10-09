# Guía de Compilación de Buildroot para BeagleBone Black con Docker (Estructura Profesional BR2_EXTERNAL)

Este documento resume el procedimiento para configurar un entorno de desarrollo aislado utilizando **Docker** en Windows 11 Pro (vía WSL2). Se aplica el estándar profesional de la industria **`BR2_EXTERNAL`**, manteniendo tu repositorio de código (Git) y el código fuente de Buildroot como carpetas independientes y desacopladas [⭐].

---

## 1. Estructura del Volumen de Trabajo

Dentro del volumen persistente de Docker (`buildroot_vol`), el espacio de trabajo se organiza de la siguiente manera:

```text
/home/developer/buildroot-project/ (Raíz del volumen de Docker)
├── buildroot_src/         # Código fuente oficial de Buildroot (Fuera de Git)
└── mi-proyecto-beaglebone/# TU REPOSITORIO GIT (Solo contiene tus configuraciones)
```

---

## 2. Configuración del Entorno (Dockerfile)

Utilizamos el siguiente `Dockerfile` para automatizar la descarga de Buildroot y configurar un entorno seguro `no-root` bajo el usuario `developer` (necesario ya que Buildroot prohíbe compilar como superusuario).

```dockerfile
# 1. Imagen oficial de Buildroot
FROM ://gitlab.com

ENV DEBIAN_FRONTEND=noninteractive

# Cambiar a root para instalar herramientas adicionales con privilegios
USER root

# 2. Instalar herramientas útiles para el desarrollo
RUN apt-get update && apt-get install -y --no-install-recommends \
    nano \
    vim \
    sudo \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 3. Configurar usuario 'developer' con permisos sudo
RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Volver al entorno de usuario estándar
USER developer
WORKDIR /home/developer/buildroot-project

# 4. Descargar Buildroot oficial en la subcarpeta del volumen
RUN git clone --depth 1 --branch master https://github.com ./buildroot_src

CMD ["/bin/bash"]
```

---

## 3. Preparación del Repositorio Git (`BR2_EXTERNAL`)

Para que Buildroot reconozca tu carpeta como una extensión válida, debes inicializar Git y crear dos archivos obligatorios en la raíz de `mi-proyecto-beaglebone/` [⭐]:

### Archivo `external.desc`
Define el nombre interno del proyecto para generar las variables de entorno de Buildroot [⭐]:
```text
name: PROYECTO_BEAGLEBONE
desc: Configuraciones personalizadas para mi BeagleBone Black con NGINX, MQTT e IIO
```

### Archivo `external.mk`
Debe existir obligatoriamente, aunque se puede dejar vacío si no compilas paquetes propios [⭐]:
```text
# Dejar vacío o añadir Makefiles adicionales
```

### Archivo `.gitignore` imprescindible
Añádelo en la raíz de tu repositorio para evitar subir binarios o configuraciones locales:
```text
# Ignorar archivos locales generados por comandos de guardado temporales
/defconfig
```

---

## 4. Configuración del Sistema (`menuconfig`)

Para vincular Buildroot con tu repositorio Git, entra a la carpeta de Buildroot pasando el parámetro `BR2_EXTERNAL` [⭐]:

```bash
cd /home/developer/buildroot-project/buildroot_src
make BR2_EXTERNAL=/home/developer/buildroot-project/mi-proyecto-beaglebone menuconfig
```

### Carga de Hardware Base
*   Carga primero los valores por defecto de la placa: `make beaglebone_defconfig`

### Modificaciones en `menuconfig` (Paths Dinámicos) [⭐]
1.  **Ruta del `rootfs_overlay` (Superposición de archivos):**
    *   Navega a: `System configuration`
    *   En `Root filesystem overlay directories`, escribe [⭐]:
        `$(BR2_EXTERNAL_PROYECTO_BEAGLEBONE_PATH)/board/rootfs_overlay`
2.  **Ruta de la configuración del Kernel:**
    *   Navega a: `Kernel`
    *   En `Kernel configuration`, selecciona `Using a custom config file`.
    *   En `Configuration file path`, escribe:
        `$(BR2_EXTERNAL_PROYECTO_BEAGLEBONE_PATH)/board/linux.config`

### Selección de Paquetes Esenciales
*   **SSH (Dropbear):** `Target packages` -> `Networking applications` -> `[*] dropbear`
*   **Linux IIO (Sensores/ADC):** `Target packages` -> `Hardware handling` -> `[*] libiio` -> `[*] iio-tests`
*   **Broker MQTT (Mosquitto):** `Target packages` -> `Networking applications` -> `[*] mosquitto`
*   **Servidor Web (NGINX):** `Target packages` -> `Networking applications` -> `[*] nginx`
*   **Firewall (nftables):** `Target packages` -> `Networking applications` -> `[*] nftables`
*   **Gestor de logs (syslog-ng):** `Target packages` -> `System tools` -> `[*] syslog-ng`
*   **Red DHCP Automática:** `System configuration` -> `Network interface to configure through DHCP` -> Escribir `eth0`

---

## 5. Configuración del Kernel de Linux (`linux-menuconfig`)

Ejecuta el menú del Kernel para activar el cortafuegos y el soporte de red USB:

```bash
make linux-menuconfig
```

*   **Soporte de nftables:** `Networking support` -> `Networking options` -> `Network packet filtering framework (Netfilter)` -> `[*] Netfilter core tables support` y en `Core Netfilter Configuration` -> `[*] Netfilter nf_tables support`.
*   **Adaptadores Ethernet USB:** `Device Drivers` -> `Network device support` -> `USB Network Adapters` -> Activar `[*] Multi-purpose USB Networking Framework`, `[*] ASIX AX88179/...` y `[*] Realtek RTL8152/...`.

---

## 6. Congelar Configuración y Guardar en Git

Una vez terminadas las interfaces de configuración, ejecuta estos comandos para **extraer de forma limpia los archivos de configuración directa a tu repositorio Git**:

```bash
# 1. Salvar el defconfig de Buildroot en tu repositorio
make BR2_EXTERNAL=/home/developer/buildroot-project/mi-proyecto-beaglebone savedefconfig
mkdir -p ../mi-proyecto-beaglebone/configs
mv defconfig ../mi-proyecto-beaglebone/configs/beaglebone_custom_defconfig

# 2. Salvar la configuración del Kernel de Linux en tu repositorio
make linux-savedefconfig
mkdir -p ../mi-proyecto-beaglebone/board
mv output/build/linux-*/defconfig ../mi-proyecto-beaglebone/board/linux.config
```

Ya puedes hacer tu primer commit estable desde la carpeta `mi-proyecto-beaglebone/` (`git add . && git commit -m "Initial repeatable build profile"`).

---

## 7. Compilación y Despliegue

### Paso A: Compilar la imagen
Lanza el proceso aprovechando los hilos de tu CPU (ejemplo con 8 hilos):
```bash
make BR2_EXTERNAL=/home/developer/buildroot-project/mi-proyecto-beaglebone -j8
```

### Paso B: Extraer y Flashear
Abre una terminal de **PowerShell** en Windows 11 para sacar la imagen del volumen de Docker:
```powershell
docker cp <id_del_contenedor>:/home/developer/buildroot-project/buildroot_src/output/images/sdcard.img C:\Users\TuUsuario\Downloads\
```
Quema el archivo `sdcard.img` en una MicroSD con **BalenaEtcher**, insértala en la BeagleBone Black y arráncala manteniendo presionado el botón **S2**.
