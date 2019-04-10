#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	Bert
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
	uiseisamplspectrum.cc
	uiseisbayesclass.cc
	uiseissampleeditor.cc
	uiseisimpcubefromothersurv.cc
	uiseiscopy.cc
	uiseisexpcubepositions.cc
	uiseisfileman.cc
	uiseisfmtscale.cc
	uiseisimportodcube.cc
	uiseisioobjinfo.cc
	uiseisiosimple.cc
	uiseislinesel.cc
	uiseismmproc.cc
	uiseismulticubeps.cc
	uiseispartserv.cc
	uiseispreloadmgr.cc
	uiseisprovider.cc
	uiseispsman.cc
	uiseisrandto2dline.cc
	uiseissel.cc
	uiseisseldata.cc
	uiseissingtrcdisp.cc
	uiseisstorer.cc
	uiseissubsel.cc
	uiseistransf.cc
	uiseistrcbufviewer.cc
	uiseiswriteopts.cc
	uiseiswvltattr.cc
	uiseiswvltgen.cc
	uiseiswvltimpexp.cc
	uiseiswvltman.cc
	uistaticsdesc.cc
	uisurvseissel.cc
	uitrcpositiondlg.cc
	uiveldesc.cc
	uivelocityvolumeconversion.cc
	uiwaveletextraction.cc
	uiwaveletmatchdlg.cc
	uiwaveletsel.cc
)

set( OD_MODULE_PROGS od_ClusterProc.cc od_SeisMMBatch.cc )

OD_INIT_MODULE()
