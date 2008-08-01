/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: initgmt.cc,v 1.1 2008-08-01 08:28:27 cvsraman Exp $
________________________________________________________________________

-*/


#include "initstdgmt.h"
#include "gmtbasemap.h"
#include "gmtlocations.h"


void initStdGMTClasses()
{
    GMTBaseMap::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
}
