# This is a FSL Android Reference Design platform based on i.MX51 BBG board
# It will inherit from FSL core product which in turn inherit from Google generic

$(call inherit-product, device/imxdev/imx5x/imx5x.mk)
$(call inherit-product, vendor/google/products/gms_full.mk)

PRODUCT_PACKAGES += \
	glgps gps.imx5x.so			\

# Overrides
PRODUCT_NAME := ivy
PRODUCT_DEVICE := imx51_ivy
