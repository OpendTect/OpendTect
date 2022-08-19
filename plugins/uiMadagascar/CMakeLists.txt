#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS uiODMain Madagascar)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uimadagascarmain.cc
	uimadbldcmd.cc
	uimadiosel.cc
	uimadpi.cc
)

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()


#ifdef MAD_UIMSG_IF_FAIL
#	OWNC++FLAGS := -DMAD_UIMSG_IF_FAIL
#endif
