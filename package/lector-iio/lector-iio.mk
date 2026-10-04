################################################################################
#
# lector-iio
#
################################################################################

LECTOR_IIO_VERSION = 1.0
LECTOR_IIO_SITE = $(BR2_EXTERNAL_BBB_PATH)/package/lector-iio/src
LECTOR_IIO_SITE_METHOD = local

LECTOR_IIO_DEPENDENCIES = libiio
LECTOR_IIO_CONF_OPTS = -DCMAKE_BUILD_TYPE=Release

$(eval $(cmake-package))

