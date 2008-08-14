/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: initgmt.cc,v 1.3 2008-08-14 10:52:47 cvsraman Exp $
________________________________________________________________________

-*/


#include "initstdgmt.h"
#include "gmtbasemap.h"
#include "gmtlocations.h"
#include "gmtcontour.h"
#include "gmtcoastline.h"
#include "gmt2dlines.h"


void initStdGMTClasses()
{
    GMTBaseMap::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
    GMTCoastline::initClass();
    GMTWells::initClass();
    GMT2DLines::initClass();
}
