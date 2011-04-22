#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	Mar 2004	Nanne Hemstra
#	RCS :		$Id: Makefile,v 1.51 2011-04-22 15:57:43 cvskris Exp $
#_______________________________________________________________________________


SRC.cc	:=	initsood.cc \
		SbImagei32.cc \
		SoOD.cc \
		SoArrow.cc \
		SoAxes.cc \
		SoBeachBall.cc \
		SoCameraFacingZAxisRotation.cc \
		SoCameraInfo.cc \
		SoCameraInfoElement.cc \
		SoColTabMultiTexture2.cc \
		SoColTabTextureChannel2RGBA.cc \
		SoDepthTabPlaneDragger.cc \
		SoDGBDragPointDragger.cc \
		SoDGBIndexedPointSet.cc \
		SoForegroundTranslation.cc \
		SoGridSurfaceDragger.cc \
		SoIndexedLineSet3D.cc \
		SoIndexedTriangleFanSet.cc \
		SoInvisibleLineDragger.cc \
		SoLineSet3D.cc \
		SoLockableSeparator.cc \
		SoManLevelOfDetail.cc \
		SoMFImage.cc \
		SoOneSideRender.cc \
		SoPerspectiveSel.cc \
		SoPlaneWellLog.cc \
		SoPolygonSelect.cc \
		SoRandomTrackLineDragger.cc \
		SoRGBATextureChannel2RGBA.cc \
		SoScale3Dragger.cc \
		SoShaderTexture2.cc \
		SoShapeScale.cc \
		SoSplitTexture2.cc \
		SoSplitTexture2Element.cc \
		SoText2Set.cc \
		SoTextureChannelSet.cc \
		SoTextureChannelSetElement.cc \
	 	SoTextureComposer.cc \
		SoTextureComposerElement.cc \
		SoTranslateRectangleDragger.cc \
		UTMCamera.cc \
		UTMElement.cc \
		UTMPosition.cc \
		ViewportRegion.cc \
		LegendKit.cc
		

PROG.cc := oib2a.cc


USECOIN := yes
USEQT   := yes

ifeq ($(HDIR),win)
    USE_SIM_VOL := Yes
endif

ifndef USE_SIM_VOL
    EXTRALIBSFORSHLIB := SimVoleon
endif

include make.od.Defaults
include make.Targets
