#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiTools CRS )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES 
	uicrssystem.cc
	uicrspi.cc
)

if ( NOT OD_NO_PROJ )
    set ( OD_PLUGIN_ALO_EXEC
	${OD_MAIN_EXEC}
	${DGB_PRO_UIEXECS}
	${DGB_ML_UIEXECS}
	${DGB_SR_EXECS}
    )
endif()

OD_INIT_MODULE()
