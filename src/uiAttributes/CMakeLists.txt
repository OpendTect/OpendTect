#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiPreStackProcessing Attributes )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
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
	uigapdeconattrib.cc
	uigdexamacorr.cc
	uiinstantattrib.cc
	uimatchdeltaattrib.cc
	uimathattrib.cc
	uimultoutsel.cc
	uipositionattrib.cc
	uiprestackattrib.cc
	uireferenceattrib.cc
	uireliefattrib.cc
	uirgbattrseldlg.cc
	uisamplevalueattrib.cc
	uiscalingattrib.cc
	uisemblanceattrib.cc
	uisetpickdirs.cc
	uishiftattrib.cc
	uisimilarityattrib.cc
	uispecdecompattrib.cc
	uisteeringsel.cc
	uistoredattrreplacer.cc
	uitextureattrib.cc
	uivolstatsattrib.cc
)

OD_INIT_MODULE()
