FILE(GLOB startactive_modules *.module)

install(
    FILES ${startactive_modules}
    DESTINATION ${DATA_INSTALL_DIR}/startactive/modules/
)

