# This is a FSL Android Reference Design platform based on i.MX51 BBG board
# It will inherit from FSL core product which in turn inherit from Google generic

$(call inherit-product, device/imxdev/imx5x/imx5x.mk)

# Overrides
PRODUCT_NAME := aster7
PRODUCT_DEVICE := imx51_aster7
