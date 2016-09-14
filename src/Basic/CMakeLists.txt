#_______________________CMake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#_______________________________________________________________________________


set( OD_USEQT Core )

if ( UNIX )
    option ( OD_ENABLE_MEMDEBUGGER_CODE
	    "Enable code that gives less false memory violation positives" OFF )
    if ( OD_ENABLE_MEMDEBUGGER_CODE )
	add_definitions("-DmMEM_DEBUGGER")
    endif()
endif( UNIX )

if ( OD_ENABLE_BREAKPAD )
    list(APPEND OD_MODULE_INCLUDESYSPATH ${BREAKPAD_DIR}/include/breakpad )
    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_BREAKPADLIBS} )
endif()

set( QT_MOC_HEADERS
	qtimercomm.h
)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/qpaths.cc.in ${CMAKE_CURRENT_BINARY_DIR}/qpaths.cc @ONLY )

set( OD_TEST_PROGS
	basic.cc
	callback.cc
	commandlineparser.cc
	datapack.cc
	dbkey.cc
	file.cc
	geometry.cc
	globexpr.cc
	math2.cc
	memcopying.cc
	od_iostream.cc
	oscommand.cc
	refcount.cc
	sets.cc
	string.cc
	svnversion.cc
	trckeyzsampling.cc
	threads.cc
	threadwork.cc
	uistring.cc
)

set( OD_MODULE_SOURCES
	applicationdata.cc
	arrayndinfo.cc
	arrayndslice.cc
	ascstream.cc
	ascbinstream.cc
	bindatadesc.cc
	bufstring.cc
	callback.cc
	color.cc
	compoundkey.cc
	commandlineparser.cc
	dateinfo.cc
	datapack.cc
	dbkey.cc
	debug.cc
	dirlist.cc
	enums.cc
	executor.cc
	factory.cc
	file.cc
	filepath.cc
	filespec.cc
	genc.cc
	gpuprog.cc
	globexpr.cc
	iopar.cc
	initbasic.cc
	latlong.cc
	legal.cc
	math2.cc
	moddepmgr.cc
	monitor.cc
	namedobj.cc
	objdisposer.cc
	od_iostream.cc
	oddirs.cc
	odimage.cc
	odinst.cc
	odmemory.cc
	odstring.cc
	odver.cc
	oscommand.cc
	plugins.cc
	posinfo.cc
	posinfo2d.cc
	posinfo2dsurv.cc
	position.cc
	progressmeter.cc
	qstreambuf.cc
	${CMAKE_CURRENT_BINARY_DIR}/qpaths.cc
	refcount.cc
	repos.cc
	rowcol.cc
	safefileio.cc
	separstr.cc
	settings.cc
	settingsaccess.cc
	sighndl.cc
	string2.cc
	strmoper.cc
	strmprov.cc
	survgeom.cc
	survinfo.cc
	task.cc
	thread.cc
	threadwork.cc
	texttranslator.cc
	timefun.cc
	timer.cc
	trckeyzsampling.cc
	uistring.cc
	uistrings.cc
	winutils.cc
	zdomain.cc
)

set ( OD_MODULE_PROGS test_exit_program.cc )

if ( ITTNOTIFY_FOUND )
    set ( OD_MODULE_DEPS ${ITTNOTIFY_LIBRARY} )
    list ( APPEND OD_MODULE_INCLUDESYSPATH ${ITTNOTIFY_INCLUDE_DIR} )
endif()

if ( NOT (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR) )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/testscripts/script with space.cmd"
                    "${PROJECT_BINARY_DIR}/testscripts/script with space.cmd" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/testscripts/script with space.sh"
                    "${PROJECT_BINARY_DIR}/testscripts/script with space.sh" )
    execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/testscripts/count_to_1000.csh"
                    "${PROJECT_BINARY_DIR}/testscripts/count_to_1000.csh" )

endif()

OD_INIT_MODULE()

