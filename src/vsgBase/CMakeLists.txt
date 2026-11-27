#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS visBase )
set( OD_FOLDER "Visualization (3D)" )
set( OD_USEVSG TRUE )

if ( NOT ( TARGET Vulkan::Vulkan AND TARGET vsg::vsg ) )
    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_MODULES ${OD_MODULE_NAME} )
    set( OD_MODULES ${OD_MODULES} PARENT_SCOPE )
    return()
endif()

set( OD_MODULE_SOURCES
	initvsgbase.cc
	vsggeoaxes.cc
	vsggeocolorlookup.cc
	vsggeocompiler.cc
	vsggeogradientbackground.cc
	vsggeolayeredtexture.cc
	vsggeolines.cc
	vsggeomarkerset.cc
	vsggeonearfar.cc
	vsggeoonesiderender.cc
	vsggeopick.cc
	vsggeoplanedragger.cc
	vsggeoscalarbar.cc
	vsggeotext.cc
	vsggeotextureplane.cc
	vsggeothumbwheel.cc
	vsggeotrackball.cc
	vsggeotriangles.cc
	vsggeoupdater.cc
	visvsgannotation.cc
	visvsgaxes.cc
	visvsgcamera.cc
	visvsgcoltablookup.cc
	visvsgdata.cc
	visvsgdatagroup.cc
	visvsgdataman.cc
	visvsgevent.cc
	visvsgfontcreator.cc
	visvsgmarkerset.cc
	visvsgobject.cc
	visvsgplanedragger.cc
	visvsgscenecoltab.cc
	visvsgselman.cc
	visvsgtexturechannels.cc
	visvsgtexturerectangle.cc
	visvsgtext.cc
	visvsgthumbwheel.cc
	visvsgtransform.cc
)

set( OD_TEST_PROGS
	vsggeo.cc
	vsggeoaxes.cc
	vsggeocolorlookup.cc
	vsggeogradientbackground.cc
	vsggeopick.cc
	vsggeoscalarbar.cc
	vsggeotext.cc
	vsggeotextureplane.cc
	vsggeotextperf.cc
	vsggeothumbwheel.cc
	vsggeotrackball.cc
	visvsgannotation.cc
	visvsgaxes.cc
	visvsgcoltablookup.cc
	visvsgcontrastcolor.cc
	visvsgevent.cc
	visvsgfontcreator.cc
	visvsgobjects.cc
	visvsgplanedragger.cc
	visvsgscenecoltab.cc
	visvsgtexturechannels.cc
	visvsgtexturerectangle.cc
	visvsgtext.cc
	visvsgthumbwheel.cc
)
if ( NOT OD_NO_VSG )
    list( APPEND OD_TEST_PROGS visvsgtransform.cc )
endif()
set( GPUTESTS
	test_vsggeo
	test_vsggeoaxes
	test_vsggeogradientbackground
	test_vsggeopick
	test_vsggeoscalarbar
	test_vsggeotext
	test_vsggeotextureplane
	test_vsggeotextperf
	test_vsggeothumbwheel
	test_visvsgobjects
	test_visvsgfontcreator
	test_visvsgannotation
	test_visvsgtext
	test_visvsgtexturerectangle
	test_visvsgaxes
)

if ( UNIX )
    if ( NOT TARGET Fontconfig::Fontconfig OR NOT TARGET Freetype::Freetype )
	message( SEND_ERROR
	    "Fontconfig and Freetype are required to build text on Unix" )
    else()
	list( APPEND OD_MODULE_EXTERNAL_LIBS
	      Fontconfig::Fontconfig Freetype::Freetype )
    endif()
endif()

OD_INIT_MODULE()

foreach( GPUTEST ${GPUTESTS} )
    if ( TEST ${GPUTEST} )
	set_property( TEST ${GPUTEST} APPEND PROPERTY LABELS gpu )
    endif()
endforeach()
