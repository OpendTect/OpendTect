#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_FOLDER "Base" )
set( OD_USEQT Core )
set( OD_USEBREAKPAD TRUE )

if ( UNIX )
    option( OD_ENABLE_MEMDEBUGGER_CODE
	    "Enable code that gives less false memory violation positives" OFF )
    if ( OD_ENABLE_MEMDEBUGGER_CODE )
	add_definitions("-DmMEM_DEBUGGER")
    endif()
endif( UNIX )
if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/odpy" )
    add_definitions( -D__odpy_dir__="${CMAKE_SOURCE_DIR}/external/odpy" )
endif()

set( OD_TEST_PROGS
	basic.cc
	callback.cc
	commandlaunchmgr.cc
	commandlineparser.cc
	datapack.cc
	file.cc
	geometry.cc
	globexpr.cc
	iopar.cc
	latlong.cc
	math2.cc
	memcopying.cc
	od_iostream.cc
	odjson.cc
	oscommand.cc
	pythoncomm.cc
	ranges.cc
	refcount.cc
	sets.cc
	string.cc
	svnversion.cc
	trckeyzsampling.cc
	threads.cc
	threadwork.cc
	uistring.cc
	various_basic.cc
)

if ( UNIX )
    list( APPEND OD_TEST_PROGS gason.cc )
endif()

set( OD_MODULE_SOURCES
	applicationdata.cc
	arrayndinfo.cc
	arrayndslice.cc
	ascbinstream.cc
	ascstream.cc
	bindatadesc.cc
	bufstring.cc
	callback.cc
	color.cc
	commanddefs.cc
	commandlaunchmgr.cc
	commandlineparser.cc
	compoundkey.cc
	coordsystem.cc
	datapack.cc
	dateinfo.cc
	dbkey.cc
	debug.cc
	dirlist.cc
	enums.cc
	executor.cc
	factory.cc
	file.cc
	filepath.cc
	filespec.cc
	filesystemaccess.cc
	filesystemwatcher.cc
	gason.cpp
	genc.cc
	globexpr.cc
	iopar.cc
	latlong.cc
	legal.cc
	math2.cc
	moddepmgr.cc
	monitor.cc
	multiid.cc
	namedmonitoredobject.cc
	namedobj.cc
	objdisposer.cc
	od_iostream.cc
	odcommonenums.cc
	oddirs.cc
	odimage.cc
	odinst.cc
	odjson.cc
	odmemory.cc
	odstring.cc
	odver.cc
	oscommand.cc
	plugins.cc
	posinfo.cc
	posinfo2d.cc
	posinfo2dsurv.cc
	position.cc
	procdescdata.cc
	progressmeter.cc
	pythonaccess.cc
	qstreambuf.cc
	refcount.cc
	repos.cc
	rowcol.cc
	safefileio.cc
	separstr.cc
	settings.cc
	settingsaccess.cc
	sharedobject.cc
	sighndl.cc
	string2.cc
	stringbuilder.cc
	strmoper.cc
	strmprov.cc
	surveydisklocation.cc
	survgeom.cc
	survgeom2d.cc
	survinfo.cc
	task.cc
	texttranslator.cc
	thread.cc
	threadwork.cc
	timefun.cc
	timer.cc
	trckeysampling.cc
	trckeyzsampling.cc
	uistring.cc
	uistrings.cc
	uistringset.cc
	winutils.cc
	zdomain.cc
	initbasic.cc
)

if ( ITTNOTIFY_FOUND )
    set ( OD_MODULE_DEPS ${ITTNOTIFY_LIBRARY} )
    list ( APPEND OD_MODULE_INCLUDESYSPATH ${ITTNOTIFY_INCLUDE_DIR} )
endif()

if ( NOT (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR) )
    if ( UNIX )
	set( FILEEXT sh )
    else()
	set( FILEEXT cmd )
    endif()
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/bin/process_dumpfile.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/bin/process_dumpfile.${FILEEXT}" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/testscripts/script with space.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/testscripts/script with space.${FILEEXT}" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/testscripts/count_to_1000.csh"
                    "${PROJECT_BINARY_DIR}/testscripts/count_to_1000.csh" )

    if ( UNIX )
	set( FILEEXT sh )
    else()
	set( FILEEXT bat )
    endif()
    execute_process(
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		    "${CMAKE_SOURCE_DIR}/bin/od_external.${FILEEXT}"
		    "${PROJECT_BINARY_DIR}/bin/od_external.${FILEEXT}" )
endif()

OD_INIT_MODULE()
