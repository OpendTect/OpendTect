#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define dgbbase package variables
# Author:       Nageswara
# Date:         Nov 2012
#RCS:           $Id$

#//TODO Modify script to work on all platforms.
SET( LIBLIST dGBCommon dGBUsage dGBPreStack uidGBPreStack uidGBCommon )
#TODO copy libs from relbase once it is added to od
#SET( LIBLIST ".dgb.lmgrd.vars,.start.dgb.lmgrd,.install.dgb.license,odinit.dgb, dGBCommon,dGBUsage,uidGBPreStack,dGBPreStack,uidGBCommon" )
SET( EXECLIST  )
SET( PACK "dgbbase" )
