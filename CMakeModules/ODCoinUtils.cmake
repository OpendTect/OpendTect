#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODCoinUtils.cmake,v 1.1 2012-01-24 14:09:57 cvskris Exp $
#_______________________________________________________________________________

SET( OD_COINDIR_ENV $ENV{OD_COINDIR})

IF(OD_COINDIR_ENV)
    SET(COINDIR ${OD_COINDIR_ENV})
ELSE()
    SET(COINDIR "" CACHE PATH "COINDIR location")
ENDIF()

MACRO(OD_SETUP_COIN)
    IF(OD_USECOIN)
        LIST(APPEND MODULE_INCLUDEPATH ${COINDIR}/include )
	FIND_LIBRARY(COINLIB NAMES Coin PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(SOQTLIB NAMES SoQt PATHS ${COINDIR}/lib REQUIRED )
	FIND_LIBRARY(GLLIB NAMES gl )

	SET(TMPVAR ${CMAKE_FIND_LIBRARY_SUFFIXES})
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${OD_STATIC_EXTENSION})
	FIND_LIBRARY(SIMVOLEONLIB NAMES SimVoleon PATHS ${COINDIR}/lib REQUIRED )
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${TMPVAR})

        SET(OD_COIN_LIBS ${COINLIB} ${SOQTLIB} ${GLLIB} )
    ENDIF()
ENDMACRO(OD_SETUP_COIN)
