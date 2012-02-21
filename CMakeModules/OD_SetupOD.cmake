SET( INCLUDES
    ODUtils
    ODQtUtils
    ODZlibUtils
    ODCoinUtils
    ODOsgUtils
    ODPlatformUtils
    ODMacroUtils
    ODModDeps
    CreateLaunchers
)

FOREACH( INC ${INCLUDES} )
    INCLUDE( ${OpendTect_DIR}/CMakeModules/${INC}.cmake )
ENDFOREACH()

FILE ( GLOB ALOFILES ${CMAKE_BINARY_DIR}/plugins/${OD_PLFSUBDIR}/*.${OD_SUBSYSTEM}.alo )
FOREACH( ALOFILE ${ALOFILES} )
    FILE ( REMOVE ${ALOFILE} )
ENDFOREACH()
