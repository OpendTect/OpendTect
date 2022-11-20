#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS AttributeEngine )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	maddefs.cc
	madio.cc
	madpi.cc
	madproc.cc
	madprocexec.cc
	madprocflow.cc
	madstream.cc
)
set( OD_MODULE_BATCHPROGS od_madexec.cc )

OD_INIT_MODULE()
