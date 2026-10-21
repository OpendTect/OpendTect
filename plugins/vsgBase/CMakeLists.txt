#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General visBase )
set( OD_FOLDER "Visualization (3D)" )
set( OD_IS_PLUGIN yes )
set( OD_NO_ALO_ENTRY TRUE )
set( OD_MODULE_SOURCES
	vsggeoaxes.cc
	vsggeocompiler.cc
	vsggeolines.cc
	vsggeomarkerset.cc
	vsggeonearfar.cc
	vsggeoonesiderender.cc
	vsggeopick.cc
	vsggeoplanedragger.cc
	vsggeoscalarbar.cc
	vsggeotext.cc
	vsggeothumbwheel.cc
	vsggeotrackball.cc
	vsggeotriangles.cc
	vsggeoupdater.cc
	visvsgannotation.cc
	visvsgaxes.cc
	visvsgcamera.cc
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
	visvsgtext.cc
	visvsgthumbwheel.cc
	visvsgtransform.cc
)
set( OD_TEST_PROGS
	vsggeo.cc
	vsggeoaxes.cc
	vsggeopick.cc
	vsggeoscalarbar.cc
	vsggeotext.cc
	vsggeotextperf.cc
	vsggeothumbwheel.cc
	vsggeotrackball.cc
	visvsgannotation.cc
	visvsgaxes.cc
	visvsgevent.cc
	visvsgfontcreator.cc
	visvsgobjects.cc
	visvsgplanedragger.cc
	visvsgscenecoltab.cc
	visvsgtext.cc
	visvsgthumbwheel.cc
	visvsgtransform.cc
)
set( GPUTESTS
	test_vsggeo
	test_vsggeoaxes
	test_vsggeopick
	test_vsggeoscalarbar
	test_vsggeotext
	test_vsggeotextperf
	test_vsggeothumbwheel
	test_visvsgobjects
	test_visvsgfontcreator
	test_visvsgannotation
	test_visvsgtext
	test_visvsgaxes
)

OD_USE_VSG()

if ( UNIX )
    if ( NOT TARGET Fontconfig::Fontconfig OR NOT TARGET Freetype::Freetype )
	message( SEND_ERROR
	    "Fontconfig and Freetype are required to build text on Linux" )
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
