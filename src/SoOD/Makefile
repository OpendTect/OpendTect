#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	Mar 2004	Nanne Hemstra
#	RCS :		$Id: Makefile,v 1.9 2005-05-30 14:54:56 cvsdgb Exp $
#_______________________________________________________________________________


SRC.cc	:=	SoCameraInfo.cc \
		SoCameraInfoElement.cc \
		SoDepthTabPlaneDragger.cc \
		SoForegroundTranslation.cc \
		SoIndexedTriangleFanSet.cc \
		SoLODMeshSurface.cc \
		SoManLevelOfDetail.cc \
		SoPerspectiveSel.cc \
		SoPlaneWellLog.cc \
		SoShapeScale.cc \
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

cppcompmeshsurf := $(P_COMPILE.cc)
ifeq ($(HOST),$(mac))
	cppcompmeshsurf := $(subst -O2,,$(cppcompmeshsurf))
endif

include make.Targets

$(LIBWORK)(SoLODMeshSurface.o) : SoLODMeshSurface.cc
	@echo "Compiling $< ... " $(TEE)
	@-$(cppcompmeshsurf) $< -o $(TMPPREFIX)$*.o $(OUT)
	@-$(AR) $(ARFLAGS) $@ $(TMPPREFIX)$*.o $(OUT)
	@-$(RM) $(TMPPREFIX)$*.o
