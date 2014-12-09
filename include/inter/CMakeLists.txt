INSTALL_FILES(/include/inter "\\.h$")

IF(APPLE)
    ADD_DEFINITIONS(darwin)
ENDIF(APPLE)
