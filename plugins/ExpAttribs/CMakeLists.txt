#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	corrmultiattrib.cc
	expattribspi.cc
	eventfreqattrib.cc
	grubbsfilterattrib.cc
	integratedtrace.cc
	similaritybyaw.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )

OD_INIT_MODULE()
