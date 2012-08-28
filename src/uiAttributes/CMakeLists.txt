#_______________________Pmake___________________________________________________
#
#	Makefile : 	Attribute-related UI
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-08-28 13:24:21 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiPreStackProcessing uiVolumeProcessing Attributes )
SET(OD_MODULE_SOURCES
		attribsetcreator.cc
		inituiattributes.cc
		uiattr2dsel.cc
		uiattrdesced.cc
		uiattrdescseted.cc
		uiattrgetfile.cc
		uiattribcrossplot.cc
		uiattribfactory.cc
		uiattribpanel.cc
		uiattribpartserv.cc
		uiattribsetbuild.cc
		uiattribsingleedit.cc
		uiattrinpdlg.cc
		uiattrsel.cc
		uiattrsetman.cc
		uiattrtypesel.cc
		uiattrvolout.cc
		uiautoattrdescset.cc
		uiconvolveattrib.cc
		uicrossattrevaluatedlg.cc
		uideltaresampleattrib.cc
		uidipfilterattrib.cc
		uienergyattrib.cc
		uievaluatedlg.cc
		uieventattrib.cc
		uifingerprintattrib.cc
		uifingerprintcalcobj.cc
		uifreqfilterattrib.cc
		uifrequencyattrib.cc
		uigainanalysisdlg.cc
		uiinstantattrib.cc
		uimatchdeltaattrib.cc
		uimathattrib.cc
		uimultoutsel.cc
		uipositionattrib.cc
		uiprestackattrib.cc
		uireferenceattrib.cc
		uisamplevalueattrib.cc
		uiscalingattrib.cc
		uisetpickdirs.cc
		uishiftattrib.cc
		uisimilarityattrib.cc
		uispecdecompattrib.cc
		uisteeringsel.cc
		uistoredattrreplacer.cc
		uivolstatsattrib.cc
)
OD_INIT_MODULE()
