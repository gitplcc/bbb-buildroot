# 1. Imagen oficial de Buildroot
FROM registry.gitlab.com/buildroot.org/buildroot/base:20260708.2311
ENV DEBIAN_FRONTEND=noninteractive

# 2. Instalar herramientas útiles adicionales
USER root
RUN apt-get update && apt-get install -y --no-install-recommends \
    nano \
    vim \
    sudo \
    fdisk \
    pre-commit \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# 3. Configurar el usuario 'developer'
# RUN useradd -m -s /bin/bash developer && \
#     echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
RUN echo "br-user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER br-user
WORKDIR /home/br-user/bbb-buildroot-project

RUN mkdir -p -m 0700 $HOME/.ssh \
    && ssh-keyscan github.com >> $HOME/.ssh/known_hosts

ENTRYPOINT ["entrypoint.sh"]
CMD ["bash"]

