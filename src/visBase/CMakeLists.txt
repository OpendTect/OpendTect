#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Geometry )
set( OD_FOLDER "Base" )
set( OD_USEOSG TRUE )

set( OD_MODULE_SOURCES
	initvisbase.cc
	visannot.cc
	visaxes.cc
	visboxdragger.cc
	viscamera.cc
	viscoord.cc
	visdata.cc
	visdatagroup.cc
	visdataman.cc
	visdepthtabplanedragger.cc
	visdragger.cc
	visdrawstyle.cc
	visevent.cc
	visflatviewer.cc
	visgeomindexedshape.cc
	visgridlines.cc
	vishordatahandler.cc
	vishorizonsection.cc
	vishorizonsectiontile.cc
	vishorizonsectiontileglue.cc
	vishorizontexturehandler.cc
	vishorthreadworks.cc
	vishortileresolutiondata.cc
	vishortilescreatorandupdator.cc
	visimagerect.cc
	vislight.cc
	vislines.cc
	vismarchingcubessurface.cc
	vismarkerset.cc
	vismaterial.cc
	visnodestate.cc
	visnormals.cc
	visobject.cc
	visosg.cc
	vispointset.cc
	vispolygonoffset.cc
	vispolygonselection.cc
	vispolyline.cc
	visrandompos2body.cc
	visrandomtrackdragger.cc
	visrgbatexturechannel2rgba.cc
	visscalebar.cc
	visscene.cc
	visscenecoltab.cc
	visselman.cc
	visshape.cc
	vistext.cc
	vistexturechannel2rgba.cc
	vistexturechannel2voldata.cc
	vistexturechannels.cc
	vistexturecoords.cc
	vistexturepanelstrip.cc
	vistexturerect.cc
	visthumbwheel.cc
	vistopbotimage.cc
	vistransform.cc
	vistristripset.cc
	visvolorthoslice.cc
	visvolrenscalarfield.cc
	viswell.cc
)

OD_INIT_MODULE()
if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
    OD_OSGGEO_COPYSYMBOLS( ${OD_MODULE_NAME} )
endif()
