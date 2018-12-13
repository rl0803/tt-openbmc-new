require conf/distro/include/phosphor-defaults.inc

# Override these in poky based distros
POKY_DEFAULT_DISTRO_FEATURES = "largefile opengl ptest multiarch wayland vulkan"
POKY_DEFAULT_EXTRA_RDEPENDS = "packagegroup-core-boot"
POKY_DEFAULT_EXTRA_RRECOMMENDS = "kernel-module-af-packet"

DISTRO_EXTRA_RDEPENDS += " ${POKY_DEFAULT_EXTRA_RDEPENDS}"
DISTRO_EXTRA_RRECOMMENDS += " ${POKY_DEFAULT_EXTRA_RRECOMMENDS}"

POKYQEMUDEPS = "${@bb.utils.contains("INCOMPATIBLE_LICENSE", "GPL-3.0", "", "packagegroup-core-device-devel",d)}"
DISTRO_EXTRA_RDEPENDS_append_qemuarm = " ${POKYQEMUDEPS}"
DISTRO_EXTRA_RDEPENDS_append_qemuarm64 = " ${POKYQEMUDEPS}"
DISTRO_EXTRA_RDEPENDS_append_qemumips = " ${POKYQEMUDEPS}"
DISTRO_EXTRA_RDEPENDS_append_qemuppc = " ${POKYQEMUDEPS}"
DISTRO_EXTRA_RDEPENDS_append_qemux86 = " ${POKYQEMUDEPS}"
DISTRO_EXTRA_RDEPENDS_append_qemux86-64 = " ${POKYQEMUDEPS}"

TCLIBCAPPEND = ""

QEMU_TARGETS ?= "arm aarch64 i386 mips mipsel mips64 mips64el nios2 ppc x86_64"

PREMIRRORS ??= "\
bzr://.*/.*   http://downloads.yoctoproject.org/mirror/sources/ \n \
cvs://.*/.*   http://downloads.yoctoproject.org/mirror/sources/ \n \
git://.*/.*   http://downloads.yoctoproject.org/mirror/sources/ \n \
gitsm://.*/.* http://downloads.yoctoproject.org/mirror/sources/ \n \
hg://.*/.*    http://downloads.yoctoproject.org/mirror/sources/ \n \
osc://.*/.*   http://downloads.yoctoproject.org/mirror/sources/ \n \
p4://.*/.*    http://downloads.yoctoproject.org/mirror/sources/ \n \
svn://.*/.*   http://downloads.yoctoproject.org/mirror/sources/ \n"

MIRRORS =+ "\
ftp://.*/.*      http://downloads.yoctoproject.org/mirror/sources/ \n \
http://.*/.*     http://downloads.yoctoproject.org/mirror/sources/ \n \
https://.*/.*    http://downloads.yoctoproject.org/mirror/sources/ \n"

# The CONNECTIVITY_CHECK_URI's are used to test whether we can successfully
# fetch from the network (and warn you if not). To disable the test set
# the variable to be empty.
# Git example url: git://git.yoctoproject.org/yocto-firewall-test;protocol=git;rev=master
CONNECTIVITY_CHECK_URIS ?= "https://www.example.com/"

SANITY_TESTED_DISTROS ?= " \
            poky-2.4 \n \
            poky-2.5 \n \
            ubuntu-15.04 \n \
            ubuntu-16.04 \n \
            ubuntu-16.10 \n \
            ubuntu-17.04 \n \
            fedora-26 \n \
            centos-7 \n \
            debian-8 \n \
            debian-9 \n \
            opensuse-42.1 \n \
            opensuse-42.2 \n \
            "
#
# OELAYOUT_ABI allows us to notify users when the format of TMPDIR changes in
# an incompatible way. Such changes should usually be detailed in the commit
# that breaks the format and have been previously discussed on the mailing list
# with general agreement from the core team.
#

OELAYOUT_ABI = "12"

# QA check settings - a little stricter than the OE-Core defaults
WARN_TO_ERROR_QA = "already-stripped compile-host-path install-host-path \
                    installed-vs-shipped ldflags pn-overrides rpaths staticdev \
                    useless-rpaths"
WARN_QA_remove = "${WARN_TO_ERROR_QA}"
ERROR_QA_append = " ${WARN_TO_ERROR_QA}"

require conf/distro/include/no-static-libs.inc

DISTRO = "openbmc-phosphor"
DISTRO_NAME = "Phosphor OpenBMC (Phosphor OpenBMC Project Reference Distro)"
DISTRO_VERSION = "0.1.0"
TARGET_VENDOR="-openbmc"

IMAGE_LINGUAS = "en-us"

VIRTUAL-RUNTIME_keymaps = ""

# Phosphor OpenBMC uses systemd and udev
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = ""

DISTRO_FEATURES_BACKFILL_CONSIDERED = " \
        bluez5 \
        ldconfig \
        gobject-introspection-data \
        pulseaudio \
        sysvinit \
        "

DISTRO_FEATURES_OBMC = " \
        obmc-host-ctl \
        obmc-host-ipmi \
        obmc-logging-mgmt \
        obmc-phosphor-chassis-mgmt \
        obmc-phosphor-fan-mgmt \
        obmc-phosphor-flash-mgmt \
        obmc-phosphor-system-mgmt \
        obmc-phosphor-user-mgmt \
        obmc-settings-mgmt \
        "

DISTRO_FEATURES_DEFAULT = " \
        avahi \
        pam \
        slp \
        systemd \
        ldap \
        libc-inet-anl \
        "

DISTRO_FEATURES = " \
        ${DISTRO_FEATURES_DEFAULT} \
        ${DISTRO_FEATURES_LIBC} \
        ${DISTRO_FEATURES_OBMC} \
        "

MACHINE_FEATURES += " \
        ${OBMC_MACHINE_FEATURES} \
        "

DISTRO_EXTRA_RDEPENDS_remove_qemuarm = "packagegroup-core-device-devel"

# TODO: openbmc/openbmc#12345 - Upgrade to pygobject-3.
PREFERRED_VERSION_python-pygobject = "2.%"

include conf/distro/include/openbmc-phosphor/${MACHINE}.inc

KERNEL_FEATURES_append = " phosphor-gpio-keys"
KERNEL_FEATURES_remove_qemuall = " phosphor-gpio-keys"

IMAGE_CLASSES_append = " image_types_phosphor"

# OpenBMC uses a couple recipes from meta-virtualization
# but we do not actually want any virtualization support
# enabled on a BMC distribution.  Silence the emitted
# warning.
SKIP_META_VIRT_SANITY_CHECK = "1"

# Skip the udev database by default.  It adds around 2MB
# compressed to the root filesystem, and probably doesn't
# make sense on a BMC anyway.
BAD_RECOMMENDATIONS_pn-obmc-phosphor-image = "udev-hwdb"
BAD_RECOMMENDATIONS_pn-obmc-phosphor-image += "shared-mime-info"

LAYER_CONF_VERSION ?= "8"