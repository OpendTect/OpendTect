#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	Mar 2004	Nanne Hemstra
#	RCS :		$Id: Makefile,v 1.12 2006-02-09 13:14:07 cvsnanne Exp $
#_______________________________________________________________________________


SRC.cc	:=	SoCameraInfo.cc \
		SoCameraInfoElement.cc \
		SoDepthTabPlaneDragger.cc \
		SoForegroundTranslation.cc \
		SoIndexedTriangleFanSet.cc \
		SoLODMeshSurface.cc \
		SoManLevelOfDetail.cc \
		SoMFImage.cc \
		SoMultiTexture2.cc \
		SoPerspectiveSel.cc \
		SoPlaneWellLog.cc \
		SoShapeScale.cc \
		SoText2Set.cc \
		SoTranslateRectangleDragger.cc \
		SoRandomTrackLineDragger.cc \
		SoGridSurfaceDragger.cc \
		SoArrow.cc \
		SoIndexedLineSet3D.cc \
		SoOD.cc \
		UTMCamera.cc \
		UTMElement.cc \
		UTMPosition.cc 

PROG.cc := oifileviewer.cc


USECOIN := yes
USEQT   := yes

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
