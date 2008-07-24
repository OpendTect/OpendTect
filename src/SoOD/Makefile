#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	Mar 2004	Nanne Hemstra
#	RCS :		$Id: Makefile,v 1.26 2008-07-24 20:00:04 cvskris Exp $
#_______________________________________________________________________________


SRC.cc	:=	initsood.cc \
		SoOD.cc \
		SoScale3Dragger.cc \
		SoCameraInfo.cc \
		SoCameraInfoElement.cc \
		SoCameraFacingZAxisRotation.cc \
		SoDepthTabPlaneDragger.cc \
		SoForegroundTranslation.cc \
		SoIndexedTriangleFanSet.cc \
		SoLODMeshSurface.cc \
		SoManLevelOfDetail.cc \
		SoMFImage.cc \
		SoColTabMultiTexture2.cc \
		SoPerspectiveSel.cc \
		SoPlaneWellLog.cc \
		SoPolygonSelect.cc \
		SoInvisibleLineDragger.cc \
		SoShapeScale.cc \
		SoShaderTexture2.cc \
		SoSplitTexture2.cc \
		SoSplitTexture2Element.cc \
		SoText2Set.cc \
		SoTranslateRectangleDragger.cc \
		SoRandomTrackLineDragger.cc \
		SoGridSurfaceDragger.cc \
		SoArrow.cc \
		SoIndexedLineSet3D.cc \
		UTMCamera.cc \
		UTMElement.cc \
		UTMPosition.cc 

PROG.cc := oib2a.cc


USECOIN := yes
USEQT   := yes
ifeq ($(HDIR),win)
    USE_SIM_VOL := Yes
endif

ifndef USE_SIM_VOL
    EXTRALIBSFORSHLIB := SimVoleon
endif

ifeq ($(HDIR),win)
    USE_SIM_VOL := Yes
endif

ifndef USE_SIM_VOL
    EXTRALIBSFORSHLIB := SimVoleon
endif

include make.od.Defaults

# Hack to get SoLODMeshSurface.cc non-optimised on MAC - compiler problem
# of gcc 3.3

cppcompmeshsurf := $(P_COMPILE.cc)
ifeq ($(HOST),$(mac))
	cppcompmeshsurf := $(subst -O2,,$(cppcompmeshsurf))
endif

include make.Targets

# See above: Hack
$(LIBWORK)(SoLODMeshSurface.o) : SoLODMeshSurface.cc
	@echo "Compiling $< ... " $(TEE)
	@-$(cppcompmeshsurf) $< -o $(TMPPREFIX)$*.o $(OUT)
	@-$(AR) $(ARFLAGS) $@ $(TMPPREFIX)$*.o $(OUT)
	@-$(RM) $(TMPPREFIX)$*.o
