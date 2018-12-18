FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_prepend = "file://iio-hwmon.conf \
                   "

do_install_append() {
        install -d ${D}/etc/default/obmc/hwmon/
        install -m 644 ${WORKDIR}/iio-hwmon.conf ${D}/etc/default/obmc/hwmon/iio-hwmon.conf
}

NAMES = " \
        pwm-tacho-controller@1e786000 \
        "

ITEMSFMT = "ahb/apb/{0}.conf"

ITEMS = "${@compose_list(d, 'ITEMSFMT', 'NAMES')}"

ENVS = "obmc/hwmon/{0}"
SYSTEMD_ENVIRONMENT_FILE_${PN} += "${@compose_list(d, 'ENVS', 'ITEMS')}"

       