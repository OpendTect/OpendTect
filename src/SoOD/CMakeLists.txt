#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin Extensions
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_USECOIN 1)
OD_SETUP_COIN()
SET(OD_USECOIN)
SET(OD_MODULE_SOURCES
		initsood.cc
		SbImagei32.cc
		SoOD.cc
		SoArrow.cc
		SoAxes.cc
		SoBeachBall.cc
		SoCameraFacingZAxisRotation.cc
		SoCameraInfo.cc
		SoCameraInfoElement.cc
		SoColTabMultiTexture2.cc
		SoColTabTextureChannel2RGBA.cc
		SoDepthTabPlaneDragger.cc
		SoDGBDragPointDragger.cc
		SoDGBIndexedPointSet.cc
		SoForegroundTranslation.cc
		SoGridSurfaceDragger.cc
		SoIndexedLineSet3D.cc
		SoIndexedTriangleFanSet.cc
		SoInvisibleLineDragger.cc
		SoLineSet3D.cc
		SoLockableSeparator.cc
		SoManLevelOfDetail.cc
		SoMFImage.cc
		SoOneSideRender.cc
		SoPerspectiveSel.cc
		SoPlaneWellLog.cc
		SoPolygonSelect.cc
		SoRandomTrackLineDragger.cc
		SoRGBATextureChannel2RGBA.cc
		SoScale3Dragger.cc
		SoShaderTexture2.cc
		SoShapeScale.cc
		SoSplitTexture2.cc
		SoSplitTexture2Element.cc
		SoText2Set.cc
		SoTextureChannelSet.cc
		SoTextureChannelSetElement.cc
	 	SoTextureComposer.cc
		SoTextureComposerElement.cc
		SoTranslateRectangleDragger.cc
		UTMCamera.cc
		UTMElement.cc
		UTMPosition.cc
		ViewportRegion.cc
		LegendKit.cc
)
IF ( WIN32 )
    LIST ( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_SIMVOLEON_LIBRARY} )
ELSE()
    SET( OD_MODULE_STATIC_LIBS ${OD_SIMVOLEON_LIBRARY} )
ENDIF()
OD_INIT_MODULE()
