#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS uiCmdDriver uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	#coincommands.cc
	cmddriverpi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
