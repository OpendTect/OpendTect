#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define dgbbase package variables
# Author:       Nageswara
# Date:         Nov 2012
#RCS:           $Id$

#//TODO Modify script to work on all platforms.
SET( LIBLIST dGBCommon dGBUsage dGBPreStack uidGBPreStack uidGBCommon )
SET( SPECFILES .dgb.lmgrd.vars .start.dgb.lmgrd .install.dgb.license odinit.dgb )
SET( EXECLIST  )
SET( PACK "dgbbase" )
