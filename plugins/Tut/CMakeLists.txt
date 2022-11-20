#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS AttributeEngine EarthModel VolumeProcessing Well )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutlogtools.cc
	tutvolproc.cc
	tutpi.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS} )
set( OD_NO_ALO_ENTRY TRUE )

OD_INIT_MODULE()
