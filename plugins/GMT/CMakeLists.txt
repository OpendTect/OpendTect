#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS EarthModel Seis Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	gmtpar.cc
	gmtarray2dinterpol.cc
	gmtbasemap.cc
	gmtclip.cc
	gmtcontour.cc
	gmtfault.cc
	gmtlocations.cc
	gmtcoastline.cc
	gmt2dlines.cc
	initgmt.cc
	gmtprocflow.cc
	gmtpi.cc
)
SET( OD_MODULE_BATCHPROGS od_gmtexec.cc )
SET( OD_PLUGIN_ALO_EXEC od_gmtexec )

OD_INIT_MODULE()
