#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiTools CRS )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES 
	uicrssystem.cc
	uicrspi.cc
)

if ( NOT OD_NO_PROJ )
    set ( OD_PLUGIN_ALO_EXEC
	${OD_MAIN_EXEC}
	${DGB_ML_UIEXECS}
    )
endif()

OD_INIT_MODULE()
