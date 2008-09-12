/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: initgmt.cc,v 1.6 2008-09-12 11:32:25 cvsraman Exp $
________________________________________________________________________

-*/


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
