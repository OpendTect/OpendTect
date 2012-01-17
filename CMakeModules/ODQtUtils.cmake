#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODQtUtils.cmake,v 1.5 2012-01-17 16:23:04 cvskris Exp $
#_______________________________________________________________________________

IF($ENV{OD_QTDIR})
    SET(QTDIR ENV{OD_QTDIR})
    SET(ENV{QTDIR} ${QTDIR} )
ELSE()
    IF($ENV{QTDIR})
        SET(QTDIR ENV{OD_QTDIR})
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
        LIST(APPEND ${${OD_MODULE_NAME}_INCLUDEPATH}
            ${QT_QTNETWORK_INCLUDE_DIR}
            ${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
        SET(OWN_QTLIBS ${QT_QTCORE_LIBRARY}
                       ${QT_QTNETWORK_LIBRARY})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Sql" )
        LIST(APPEND ${${OD_MODULE_NAME}_INCLUDEPATH}
            ${QT_QTSQL_INCLUDE_DIR}
            ${QTDIR}/include )
        SET(OWN_QTLIBS ${QT_QTSQL_LIBRARY})
    ENDIF()

    IF(${OD_USEQT} MATCHES "Gui")
        LIST(APPEND ${${OD_MODULE_NAME}_INCLUDEPATH}
            ${QT_QTCORE_INCLUDE_DIR}
            ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
        SET(OWN_QTLIBS ${QT_QTGUI_LIBRARY})
    ENDIF()

    IF( QT_MOC_HEADERS )
        FOREACH( HEADER ${QT_MOC_HEADERS} )
            LIST(APPEND QT_MOC_INPUT
                ${OpendTect_SOURCE_DIR}/include/${OD_MODULE_NAME}/${HEADER})
        ENDFOREACH()

        QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
    ENDIF( QT_MOC_HEADERS )
ENDMACRO(OD_SETUP_QT)
