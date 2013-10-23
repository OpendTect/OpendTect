#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin Wrapper classes
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Geometry)
SET(OD_USEOSG 1)

SET(OD_MODULE_SOURCES
	   initvisbase.cc
	   visannot.cc
	   visaxes.cc
	   visboxdragger.cc
	   viscamera.cc
	   viscolorseq.cc
	   viscolortab.cc
	   viscolortabindexer.cc
	   viscoltabmod.cc
	   viscoord.cc
	   visdata.cc
	   visdatagroup.cc
	   visdataman.cc
	   visdepthtabplanedragger.cc
	   visdragger.cc
	   visdrawstyle.cc
	   visevent.cc
	   visflatviewer.cc
	   visgridlines.cc
	   visgeomindexedshape.cc
	   vishordatahandler.cc
	   vishorizonsection.cc
	   vishorizonsectiontile.cc
	   vishorizonsectiontileglue.cc
	   vishorizontexturehandler.cc
	   vishorthreadworks.cc
	   vishortileresolutiondata.cc
	   vishortilescreatorandupdator.cc
	   vislight.cc
	   vislines.cc
	   vismarkerset.cc
	   vismarchingcubessurface.cc
	   vismaterial.cc
	   visnormals.cc
	   visnodestate.cc
	   visobject.cc
	   visosg.cc
	   visrandompos2body.cc
	   vispointset.cc
	   vispolygonoffset.cc
	   vispolygonselection.cc
	   vispolyline.cc
	   visrandomtrackdragger.cc
	   visrgbatexturechannel2rgba.cc
	   visscene.cc
	   visscenecoltab.cc
	   visselman.cc
	   visshape.cc
	   vistext.cc
	   vistexturecoords.cc
	   vistexturechannels.cc
	   vistexturechannel2rgba.cc
           vistexturechannel2voldata.cc
	   vistexturerect.cc
	   vistexturepanelstrip.cc
	   visthumbwheel.cc
	   vistopbotimage.cc
	   vistransform.cc
	   vistristripset.cc
	   visvolorthoslice.cc
	   visvolrenscalarfield.cc
	   viswell.cc
)

OD_INIT_MODULE()
