FILE(GLOB systemd_services *.service *.target)
FILE(GLOB systemd_services_in *.service.in)

SET(inFiles
    activitymanager.service
    contour.service
    kdeinit.service
    ksmserver.service
    lnusertemp-cache.service
    lnusertemp-socket.service
    lnusertemp-tmp.service
    nepomuk.service
    plasma-device.service
    setup-kde-skel.service
    startactive.service
    )

# FindKDE4Internal defines KDE4_LIBEXEC_INSTALL_DIR
# which is used by configure_file to set the right paths

FOREACH(infileName ${inFiles})
    MESSAGE(STATUS "Process file: ${infileName}")

    configure_file(${infileName}.in "${CMAKE_CURRENT_BINARY_DIR}/${infileName}" @ONLY)

    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${infileName}"
        DESTINATION ${LIB_INSTALL_DIR}/systemd/user/
        )
ENDFOREACH(infileName)

install(
    FILES ${systemd_services}
    DESTINATION ${LIB_INSTALL_DIR}/systemd/user/
)


