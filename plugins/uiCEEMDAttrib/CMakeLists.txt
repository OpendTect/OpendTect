#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes CEEMDAttrib)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uiceemdattrib.cc
	uiceemdattribpi.cc
)

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
