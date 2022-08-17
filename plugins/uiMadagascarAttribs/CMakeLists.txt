#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
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
