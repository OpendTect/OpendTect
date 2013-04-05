#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define dgbbase package variables
# Author:       Nageswara
# Date:         Nov 2012
#RCS:           $Id$

SET( LIBLIST dGBCommon dGBUsage dGBPreStack FaultAutoExtractor
	     uidGBPreStack uidGBCommon uiFaultAutoExtractor )
SET( SPECFILES .dgb.lmgrd.vars .start.dgb.lmgrd .install.dgb.license odinit.dgb )
SET( EXECLIST  od_process_extractfault )
SET( PACK "dgbbase" )
