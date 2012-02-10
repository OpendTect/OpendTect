#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODQtUtils.cmake,v 1.7 2012-02-10 15:33:05 cvskris Exp $
#_______________________________________________________________________________

SET(OD_QTDIR_ENV $ENV{OD_QTDIR})
SET(QTDIR_ENV $ENV{QTDIR})

IF(OD_QTDIR_ENV)
    SET(QTDIR ${OD_QTDIR_ENV})
    SET(ENV{QTDIR} ${QTDIR} )
ELSE()
    IF(QTDIR_ENV)
        SET(QTDIR ${QTDIR_ENV})
        SET(ENV{OD_QTDIR} ${QTDIR})
    ELSE()
        SET(QTDIR "" CACHE PATH "QT4 location")
        SET(ENV{QTDIR} ${QTDIR} )
        SET(ENV{OD_QTDIR} ${QTDIR})
    ENDIF()
ENDIF()

FIND_PACKAGE(Qt4 REQUIRED QtGui QtCore QtSql QtNetwork )

MACRO(OD_SETUP_QT)
    include(${QT_USE_FILE})

    IF(${OD_USEQT} MATCHES "Core" )
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTNETWORK_INCLUDE_DIR}
            ${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTCORE_LIBRARY_RELEASE}
                       ${QT_QTNETWORK_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Sql" )
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTSQL_INCLUDE_DIR}
            ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTSQL_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Gui")
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTCORE_INCLUDE_DIR}
            ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTGUI_LIBRARY_RELEASE})
    ENDIF()

    IF(${OD_USEQT} MATCHES "OpenGL")
	LIST(APPEND OD_MODULE_INCLUDEPATH
            ${QT_QTOPENGL_INCLUDE_DIR} ${QTDIR}/include )
        SET(OD_QT_LIBS ${QT_QTOPENGL_LIBRARY_RELEASE})
    ENDIF()

    IF( QT_MOC_HEADERS )
        FOREACH( HEADER ${QT_MOC_HEADERS} )
            LIST(APPEND QT_MOC_INPUT
                ${OpendTect_SOURCE_DIR}/include/${OD_MODULE_NAME}/${HEADER})
        ENDFOREACH()

        QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
    ENDIF( QT_MOC_HEADERS )

    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )
ENDMACRO(OD_SETUP_QT)
