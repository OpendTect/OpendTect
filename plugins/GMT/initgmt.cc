/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgmt.cc,v 1.8 2009-07-22 16:01:27 cvsbert Exp $";


#include "initgmt.h"
#include "gmtbasemap.h"
#include "gmtlocations.h"
#include "gmtcontour.h"
#include "gmtcoastline.h"
#include "gmt2dlines.h"


void GMT::initStdClasses()
{
    GMTBaseMap::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
    GMTCoastline::initClass();
    GMTWells::initClass();
    GMT2DLines::initClass();
    GMTRandLines::initClass();
    GMTCommand::initClass();
}
