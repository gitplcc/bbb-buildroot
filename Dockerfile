# 1. Imagen oficial de Buildroot
FROM registry.gitlab.com/buildroot.org/buildroot/base:20260708.2311
ENV DEBIAN_FRONTEND=noninteractive

# 2. Instalar herramientas útiles adicionales
USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
    nano \
    vim \
    sudo \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 3. Configurar el usuario 'developer'
RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER developer
WORKDIR /home/developer/buildroot-project

# 4. AUTOMATIZACIÓN: Clonar Buildroot directamente en el contenedor
RUN git clone --depth 1 --branch 2026.08 https://gitlab.com/buildroot.org/buildroot.git


CMD ["/bin/bash"]
