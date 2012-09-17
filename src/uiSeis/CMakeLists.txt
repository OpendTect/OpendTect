#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012/05/10 05:55:33 cvsranojay Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo Seis Usage Velocity )
SET(OD_MODULE_SOURCES
		inituiseis.cc		
		uibatchtime2depthsetup.cc
		uiclusterproc.cc	
		uielasticpropsel.cc	
		uifreqtaper.cc		
		uimergeseis.cc		
		uimultcomputils.cc	
		uiprestkmergedlg.cc	
		uiprestacksel.cc	
		uisegydef.cc		
		uisegydefdlg.cc		
		uisegyexamine.cc	
		uisegyexp.cc		
		uisegyimpdlg.cc		
		uisegymanip.cc		
		uisegyread.cc		
		uisegyreaddlg.cc	
		uisegyresortdlg.cc	
		uisegyscandlg.cc	
		uisegysip.cc		
		uisegytrchdrvalplot.cc	
		uiseis2dfileman.cc	
		uiseis2dgeom.cc		
		uiseis2dto3d.cc		
		uiseisbayesclass.cc	
		uiseisbrowser.cc	
		uiseiscbvsimp.cc	
		uiseiscbvsimpfromothersurv.cc
		uiseisfileman.cc	
		uiseisfmtscale.cc	
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
		uiseiswvltattr.cc	
		uiseiswvltgen.cc	
		uiseiswvltimpexp.cc	
		uiseiswvltman.cc	
		uiseiswvltsel.cc	
		uistaticsdesc.cc	
		uitrcpositiondlg.cc	
		uiveldesc.cc		
		uivelocityvolumeconversion.cc  
		uit2dvelconvselgroup.cc	
		uiwaveletextraction.cc	
)

SET( OD_MODULE_GUI_PROGS od_SeisMMBatch )
SET( OD_MODULE_PROGS od_SEGYExaminer od_ClusterProc )

OD_INIT_MODULE()
