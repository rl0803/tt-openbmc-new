FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"
SRC_URI_append = " file://processor-state.override.yml \
                   file://sol-pattern.override.yml"
