FILE(GLOB startactive_modules *.module)
FILE(GLOB startactive_modules_in *.module.in)

SET(inFiles
    kdeinit.module
    lnusertemp-socket.module
    lnusertemp-cache.module
    lnusertemp-tmp.module
    )

# FindKDE4Internal defines KDE4_LIBEXEC_INSTALL_DIR
# which is used by configure_file to set the right paths

FOREACH(infileName ${inFiles})
    MESSAGE(STATUS "Process file: ${infileName}")

    configure_file(${infileName}.in "${CMAKE_CURRENT_BINARY_DIR}/${infileName}" @ONLY)

    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${infileName}"
        DESTINATION ${DATA_INSTALL_DIR}/startactive/modules/
        )
ENDFOREACH(infileName)

install(
    FILES ${startactive_modules}
    DESTINATION ${DATA_INSTALL_DIR}/startactive/modules/
)


