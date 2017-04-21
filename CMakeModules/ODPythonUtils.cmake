#_______________________CMake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	Nanne Hemstra
#_______________________________________________________________________________

macro( OD_SETUP_PYTHON )
    find_package( PythonLibs QUIET )
    if ( PYTHONLIBS_FOUND )
	include_directories( ${PYTHON_INCLUDE_DIRS} )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${PYTHON_LIBRARIES} )
    endif()
endmacro()
