/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: initgmt.cc,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/


#include "initstdgmt.h"
#include "gmtbasemap.h"
#include "gmtlocations.h"
#include "gmtcontour.h"


void initStdGMTClasses()
{
    GMTBaseMap::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
}
