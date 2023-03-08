#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Kristofer
# DATE     : Nov 2015
#
# Takes a library or pdb - file and generates a .sym file in the 
# location where minidump_stacktrace wants it.
#
# Note that all previous symbols for library will be deleted.
#
# Inputs:
# LIBRARY - the library or the pdb-file
# SYM_DUMP_EXECUTABLE - the application to create the sym-files
#


#Check inputs
if ( NOT DEFINED LIBRARY )
    message( FATAL_ERROR "LIBRARY not defined" )
endif()

if ( NOT DEFINED SYM_DUMP_EXECUTABLE )
    message( FATAL_ERROR "SYM_DUMP_EXECUTABLE not defined" )
endif()

if ( NOT EXISTS ${LIBRARY} )
    message( FATAL_ERROR "${LIBRARY} does not exist" )
endif()

if ( NOT EXISTS ${SYM_DUMP_EXECUTABLE} )
    message( FATAL_ERROR "${SYM_DUMP_EXECUTABLE} does not exist" )
endif()

#Get library location and name
get_filename_component( LIBDIR ${LIBRARY} PATH )

if ( WIN32 )
    get_filename_component( LIBNAME ${LIBRARY} NAME_WE )
else()
    get_filename_component( LIBNAME ${LIBRARY} NAME )
endif()

set ( OUTDIR ${LIBDIR}/symbols )

if ( NOT EXISTS ${OUTDIR} )
    file( MAKE_DIRECTORY ${OUTDIR} )
endif()

if ( NOT EXISTS ${OUTDIR} )
    message( FATAL_ERROR "Cannot create ${OUTDIR}" )
endif()

#Create symbols, store them in SYMBOL STRING
execute_process( COMMAND ${SYM_DUMP_EXECUTABLE} ${LIBRARY}
		RESULT_VARIABLE RESULT
		OUTPUT_VARIABLE SYMBOL_STRING
		ERROR_VARIABLE ERRORS )

if ( NOT (${RESULT} EQUAL 0) )
    message( FATAL_ERROR "Error while running ${SYM_DUMP_EXECUTABLE} ${LIBRARY}"
			 ${ERRORS} )
endif()

#Convert symbols to a list and check for sanity and 
#checksum
string (FIND "${SYMBOL_STRING}" "\n" FIRSTLINELEN )
string (SUBSTRING "${SYMBOL_STRING}" 0 ${FIRSTLINELEN} SYMBOL_FIRSTLINE )
string (REGEX REPLACE " " ";" SYMBOL_LIST "${SYMBOL_FIRSTLINE}" )
list ( GET SYMBOL_LIST 0 MODULE_STRING )
if ( NOT "${MODULE_STRING}" STREQUAL "MODULE" )
    message( FATAL_ERROR "Invalid output from ${LIBRARY}" )
endif()

list ( GET SYMBOL_LIST 3 CHECKSUM )

set ( DIRNAME ${OUTDIR}/${LIBNAME} )
if ( WIN32 )
    set ( DIRNAME ${OUTDIR}/${LIBNAME}.pdb )
endif()

#Remove old symbols
if ( EXISTS ${DIRNAME} )
    file( REMOVE_RECURSE ${DIRNAME} )
endif()

#Write out new symbols to correct location
file ( WRITE ${DIRNAME}/${CHECKSUM}/${LIBNAME}.sym ${SYMBOL_STRING} )

if ( UNIX AND NOT APPLE )
    execute_process( COMMAND strip ${LIBRARY} )
endif()
