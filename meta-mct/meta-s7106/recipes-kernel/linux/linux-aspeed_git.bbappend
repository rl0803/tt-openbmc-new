FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://s7106.cfg \
            file://0001-S7106-Initial-dts.patch \
            file://0002-S7106-Common-sourec-files.patch \
            file://0003-Implement-Heart-Beat-LED.patch \
            "

