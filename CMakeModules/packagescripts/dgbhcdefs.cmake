#!/bin/csh
#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define dgbhc package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#//TODO Modify script to work on all platforms.
SET( LIBLIST HorizonCube uiHorizonCube uiHorizonCubeSlider )
SET( EXECLIST od_convert_chronostrat od_filter_horizoncube od_hc2steer od_process_horizoncube od_trim_hor_atfaults )
#SET( PACKAGE_DIR "/dsk/d21/nageswara/dev/buildtest/lux64" )
SET( PACK "dgbhc" )
