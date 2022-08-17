#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS AttributeEngine EarthModel VolumeProcessing Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutlogtools.cc
	tutvolproc.cc
	tutpi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS} )
SET( OD_NO_ALO_ENTRY 1 )

OD_INIT_MODULE()
