#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Mar 2016	K. Tingdahl
#_______________________________________________________________________________

find_package( OpenCL QUIET )

macro( OD_SETUP_OPENCL )
    if ( NOT OpenCL_FOUND )
	message( FATAL_ERROR "OpenCL not found" )
    endif()

    include_directories( ${OpenCL_INCLUDE_DIRS} )
    list( APPEND OD_MODULE_EXTERNAL_LIBS ${OpenCL_LIBRARY} )
    
endmacro( OD_SETUP_OPENCL )
