build_lib_example(
    NAME sample-mixture-random-variable
    SOURCE_FILES sample-mixture-random-variable.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
)

build_lib_example(
    NAME bursty-application-example
    SOURCE_FILES bursty-application-example.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
                      ${libpoint-to-point}
                      ${libinternet}
                      ${libnetwork}
)

build_lib_example(
    NAME vr-application-example
    SOURCE_FILES vr-application-example.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
                      ${libpoint-to-point}
                      ${libinternet}
                      ${libnetwork}
)

build_lib_example(
    NAME trace-file-burst-application-example
    SOURCE_FILES trace-file-burst-application-example.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
                      ${libpoint-to-point}
                      ${libinternet}
                      ${libnetwork}
)

build_lib_example(
    NAME vr-app-n-stas
    SOURCE_FILES vr-app-n-stas.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
                      ${libcore}
                      ${libwifi}
)

build_lib_example(
    NAME stats-calculator-example
    SOURCE_FILES stats-calculator-example.cc
    LIBRARIES_TO_LINK ${libvr-app}
                      ${libapplications}
                      ${libcore}
                      ${libwifi}
)
