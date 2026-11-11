################################################################################
#
# lector-iio
#
################################################################################

LECTOR_IIO_VERSION = 1.0.0
LECTOR_IIO_SITE = $(BR2_EXTERNAL_BBB_PATH)/package/lector-iio
LECTOR_IIO_SITE_METHOD = local

LECTOR_IIO_CMAKE_BACKEND = ninja

LECTOR_IIO_DEPENDENCIES = host-pkgconf cli11 libiio paho-mqtt-cpp spdlog
LECTOR_IIO_CONF_OPTS = -DCMAKE_BUILD_TYPE=Release

define LECTOR_IIO_APP_USERS
	lector-iio -1 iio -1 * - - - lector-iio daemon user
endef

define LECTOR_IIO_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(LECTOR_IIO_PKGDIR)/S99lector-iio \
		$(TARGET_DIR)/etc/init.d/S99lector-iio
endef

define LECTOR_IIO_INSTALL_INIT_SYSTEMD
	$(INSTALL) -D -m 0644 $(LECTOR_IIO_PKGDIR)/lector-iio.service \
		$(TARGET_DIR)/usr/lib/systemd/system/lector-iio.service
endef

# Create the sysfs permission helper script
ifeq ($(BR2_ROOTFS_DEVICE_CREATION_DYNAMIC_MDEV),y)
LECTOR_IIO_DEPENDENCIES += busybox

LECTOR_IIO_MDEV_CONF = $(TARGET_DIR)/etc/mdev.conf
LECTOR_IIO_IIO_HELPER = $(TARGET_DIR)/usr/libexec/iio-perms.sh
LECTOR_IIO_IIO_RULE = iio:device[0-9]+  root:iio  0660  */usr/libexec/iio-perms.sh

define LECTOR_IIO_INSTALL_MDEV_RULE
	mkdir -p $(dir $(LECTOR_IIO_IIO_HELPER))
	printf '%s\n' '#!/bin/sh' \
		'if [ "$$ACTION" = "add" ] && [ -n "$$DEVPATH" ]; then' \
		'	SYS_PATH="/sys$$DEVPATH"' \
		'	if [ -d "$$SYS_PATH" ]; then' \
		'		chgrp -R iio "$$SYS_PATH"' \
		'		chmod -R g=u "$$SYS_PATH"' \
		'	fi' \
		'fi' > $(LECTOR_IIO_IIO_HELPER)
	chmod 0755 $(LECTOR_IIO_IIO_HELPER)
	if ! grep -Fqx '$(LECTOR_IIO_IIO_RULE)' $(LECTOR_IIO_MDEV_CONF) 2>/dev/null; then \
		{ printf '%s\n' '# Grant iio group members R/W access to IIO devices and sysfs attributes' \
			'$(LECTOR_IIO_IIO_RULE)' ''; \
		  grep -Fvx -e '# Grant iio group members R/W access to IIO devices and sysfs attributes' \
			-e '$(LECTOR_IIO_IIO_RULE)' $(LECTOR_IIO_MDEV_CONF) 2>/dev/null || true; } \
			> $(LECTOR_IIO_MDEV_CONF).tmp && \
		mv $(LECTOR_IIO_MDEV_CONF).tmp $(LECTOR_IIO_MDEV_CONF); \
	fi
endef

LECTOR_IIO_TARGET_FINALIZE_HOOKS += LECTOR_IIO_INSTALL_MDEV_RULE
endif

$(eval $(cmake-package))
