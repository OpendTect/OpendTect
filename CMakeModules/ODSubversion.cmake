#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


# the FindSubversion.cmake module is part of the standard distribution
include(FindSubversion)

# extract working copy information for SOURCE_DIR into MY_XXX variables
if ( Subversion_FOUND )
    Subversion_WC_INFO( ${CMAKE_SOURCE_DIR} MY)
else()
    set ( MY_WC_REVISION 0 )
    set ( MY_WC_URL "" )
endif()

set ( INC_DIR ${CMAKE_SOURCE_DIR}/include/Basic )
set ( TMPFILE ${INC_DIR}/svnversion.h.tmp )

# write a file with the SVNVERSION define
file( WRITE ${TMPFILE} 
     "// Generated automatically by CMakeModules/ODSubversion.cmake\n"
     "//\n"
     "#ifndef mSVN_VERSION\n"
     "#define mSVN_VERSION ${MY_WC_REVISION}\n"
     "#define mSVN_URL \"${MY_WC_URL}\"\n"
     "#endif\n")

# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${TMPFILE} ${INC_DIR}/svnversion.h )
file ( REMOVE ${TMPFILE} )
