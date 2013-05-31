#_______________________Pmake___________________________________________________
#
#	Makefile: 	MATLAB plugin
# 	Feb 2012	Nanne
#	RCS:		$Id$
#_______________________________________________________________________________

SET( OD_MODULE_DEPS VolumeProcessing )
SET( OD_IS_PLUGIN yes )
SET( OD_MODULE_SOURCES
	matlabarray.cc
	matlablibmgr.cc
	matlabpi.cc
	matlabstep.cc
)

IF( DEFINED ENV{MATLABDIR} )
    LIST( APPEND OD_MODULE_INCLUDESYSPATH $ENV{MATLABDIR}/extern/include )
    LIST( APPEND OD_MODULE_EXTERNAL_LIBS $ENV{MATLABDIR}/bin/glnxa64/libmx.so $ENV{MATLABDIR}/bin/glnxa64/libmwmclmcr.so )
    ADD_DEFINITIONS( -DHAS_MATLAB )
ENDIF()

SET( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )
OD_INIT_MODULE()
