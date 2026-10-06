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
# RUN useradd -m -s /bin/bash developer && \
#     echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
RUN echo "br-user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER br-user
WORKDIR /home/br-user/bbb-buildroot-project

CMD ["/bin/bash","-li"]

