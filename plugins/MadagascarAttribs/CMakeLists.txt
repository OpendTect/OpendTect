#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS Attributes)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	madagcattrib.cc
	madattribspi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )

OD_INIT_MODULE()
