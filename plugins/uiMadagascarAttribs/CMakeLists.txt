#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS uiAttributes MadagascarAttribs)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uimadagcattrib.cc
	uimadattribspi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )

OD_INIT_MODULE()
