#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_FOLDER "Base" )
set( OD_USEQT Core )
set( OD_USEBREAKPAD TRUE )

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
	fileformat.cc
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

if ( UNIX )
    option( OD_ENABLE_MEMDEBUGGER_CODE
	    "Enable code that gives less false memory violation positives" OFF )
    if ( OD_ENABLE_MEMDEBUGGER_CODE )
	list ( APPEND OD_MODULE_COMPILE_DEFINITIONS "mMEM_DEBUGGER" )
    endif()
endif( UNIX )
if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/odpy" )
    list ( APPEND OD_MODULE_COMPILE_DEFINITIONS
	   "__odpy_dir__=\"${CMAKE_SOURCE_DIR}/external/odpy\"" )
endif()
if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/safety" )
    list ( APPEND OD_MODULE_COMPILE_DEFINITIONS
	   "__safety_dir__=\"${CMAKE_SOURCE_DIR}/external/safety\"" )
endif()
if ( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/external/marshmallow" )
    list ( APPEND OD_MODULE_COMPILE_DEFINITIONS
	   "__marshmallow_dir__=\"${CMAKE_SOURCE_DIR}/external/marshmallow/src\"" )
endif()

if ( NOT ${BUILDINSRC} )
    if ( UNIX )
	set( FILEEXT sh )
    else()
	set( FILEEXT cmd )
    endif()
    file( COPY "${CMAKE_SOURCE_DIR}/testscripts/script with space.${FILEEXT}"
	  DESTINATION "${PROJECT_BINARY_DIR}/testscripts" )
    file( COPY "${CMAKE_SOURCE_DIR}/testscripts/count_to_1000.csh"
	  DESTINATION "${PROJECT_BINARY_DIR}/testscripts" )

    if ( UNIX )
	set( FILEEXT sh )
    else()
	set( FILEEXT bat )
    endif()
    file( COPY "${CMAKE_SOURCE_DIR}/bin/od_external.${FILEEXT}"
	  DESTINATION "${PROJECT_BINARY_DIR}/bin" )
endif()

OD_INIT_MODULE()

if ( WIN32 AND NOT OD_NO_QT )
    set( QTCONFTXT "[Paths]\nPrefix=${QTDIR}" )
    add_custom_command( TARGET ${OD_MODULE_NAME} POST_BUILD
	COMMAND ECHO "[Paths]" > qt.conf
	COMMAND ECHO "Prefix=${QTDIR}" >> qt.conf
	WORKING_DIRECTORY "$<TARGET_FILE_DIR:${OD_MODULE_NAME}>"
	COMMENT "Setting up qt.conf" )
endif()
