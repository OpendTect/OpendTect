/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgmt.cc,v 1.7 2008-11-25 15:35:21 cvsbert Exp $";


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
