#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	maddefs.cc
	madio.cc
	madpi.cc
	madproc.cc
	madprocexec.cc
	madprocflow.cc
	madstream.cc
)
SET( OD_MODULE_BATCHPROGS od_madexec.cc )

OD_INIT_MODULE()
