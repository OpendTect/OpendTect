#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiIo Seis Velocity)
set(OD_MODULE_SOURCES
	inituiseis.cc
	uibatchtime2depthsetup.cc
	uiclusterproc.cc
	uielasticpropsel.cc
	uielasticmodelprov.cc
	uifkspectrum.cc
	uifreqtaper.cc
	uimergeseis.cc
	uimultcomputils.cc
	uiprestkmergedlg.cc
	uiprestacksel.cc
	uiseis2dfileman.cc
	uiseis2dfrom3d.cc
	uiseis2dgeom.cc
	uiseis2dto3d.cc
	uiseis2dto3dinterpol.cc
	uiseisamplspectrum.cc
	uiseisbayesclass.cc
	uiseisbrowser.cc
	uiseiscbvsimpfromothersurv.cc
	uiseiscopy.cc
	uiseisexpcubepositions.cc
	uiseisfileman.cc
	uiseisfmtscale.cc
	uiseisimportcbvs.cc
	uiseisioobjinfo.cc
	uiseisiosimple.cc
	uiseislinesel.cc
	uiseismmproc.cc
	uiseismulticubeps.cc
	uiseispartserv.cc
	uiseispreloadmgr.cc
	uiseispsman.cc
	uiseisrandto2dline.cc
	uiseissel.cc
	uiseissingtrcdisp.cc
	uiseissubsel.cc
	uiseistransf.cc
	uiseistrcbufviewer.cc
	uiseiswriteopts.cc
	uiseiswvltattr.cc
	uiseiswvltgen.cc
	uiseiswvltimpexp.cc
	uiseiswvltman.cc
	uiseiswvltsel.cc
	uistaticsdesc.cc
	uisynthgrp.cc
	uitrcpositiondlg.cc
	uiveldesc.cc
	uivelocityvolumeconversion.cc
	uiwaveletextraction.cc
	uiwaveletmatchdlg.cc
)

SET( OD_MODULE_PROGS od_ClusterProc.cc od_SeisMMBatch.cc )

OD_INIT_MODULE()
