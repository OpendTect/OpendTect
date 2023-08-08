#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiStrat Well )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	inituiwell.cc
	uibulkwellimp.cc
	uid2tmodelgrp.cc
	uilassip.cc
	uilaswriter.cc
	uimnemonicsettings.cc
	uimultiwelllogsel.cc
	uisimplemultiwell.cc
	uiwelldahdisplay.cc
	uiwelldisplay.cc
	uiwelldisplaycontrol.cc
	uiwelldisplaymarkeredit.cc
	uiwelldisplayserver.cc
	uiwelldispprop.cc
	uiwelldisppropdlg.cc
	uiwelldlgs.cc
	uiwellfiltergrp.cc
	uiwellimpasc.cc
	uiwellinserter.cc
	uiwelllogcalc.cc
	uiwelllogdisplay.cc
	uiwelllogimpexp.cc
	uiwelllogtools.cc
	uiwelllogtoolsgrp.cc
	uiwellman.cc
	uiwellmarkerdlg.cc
	uiwellmarkersel.cc
	uiwellpartserv.cc
	uiwellposprov.cc
	uiwellpropertyrefsel.cc
	uiwellrdmlinedlg.cc
	uiwellsel.cc
	uiwellstratdisplay.cc
	uiwellt2dconv.cc
	uiwellwriteopts.cc
	uizrangeselect.cc
	uiwelldataexport.cc
)

OD_INIT_MODULE()
OD_INSTALL_SIP()
