file (GLOB startactive_modules *.module)
file (GLOB startactive_modules_in *.module.in)

set (
   inFiles
   kdeinit.module
   )

# FindKDE4Internal defines KDE4_LIBEXEC_INSTALL_DIR
# which is used by configure_file to set the right paths

foreach (infileName ${inFiles})
   message (STATUS "Process file: ${infileName}")

   configure_file (${infileName}.in "${CMAKE_CURRENT_BINARY_DIR}/${infileName}" @ONLY)

   install (
      FILES "${CMAKE_CURRENT_BINARY_DIR}/${infileName}"
      DESTINATION ${STARTPLASMA_MODULE_DIR}
      )
endforeach ()

install (
   FILES ${startactive_modules}
   DESTINATION ${STARTPLASMA_MODULE_DIR}
   )

