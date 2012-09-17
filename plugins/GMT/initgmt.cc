/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgmt.cc,v 1.13 2012/03/19 13:24:34 cvsnageswara Exp $";

#include "gmtarray2dinterpol.h"
#include "gmtbasemap.h"
#include "gmtclip.h"
#include "gmtcoastline.h"
#include "gmtcontour.h"
#include "gmtfault.h"
#include "gmtlocations.h"
#include "gmt2dlines.h"
#include "initgmtplugin.h"


void GMT::initStdClasses()
{
    GMTBaseMap::initClass();
    GMTClip::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
    GMTFault::initClass();
    GMTCoastline::initClass();
    GMTWells::initClass();
    GMT2DLines::initClass();
    GMTRandLines::initClass();
    GMTCommand::initClass();
    GMTSurfaceGrid::initClass();
    GMTNearNeighborGrid::initClass();
}
