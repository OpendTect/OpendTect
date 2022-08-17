#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS ExpAttribs uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uieventfreqattrib.cc
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uicorrmultiattrib.cc
	uisimilaritybyaw.cc
	uiintegratedtrace.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
