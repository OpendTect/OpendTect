/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtcoastline.cc,v 1.1 2008-08-07 12:38:39 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtcoastline.h"

#include "gmtpar.h"
#include "uimsg.h"


int uiGMTCoastlineGrp::factoryid_ = -1;

void uiGMTCoastlineGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Coastline",
				    uiGMTCoastlineGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTCoastlineGrp::createInstance( uiParent* p )
{
    return new uiGMTCoastlineGrp( p );
}


uiGMTCoastlineGrp::uiGMTCoastlineGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Coastline")
{
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTCoastlineGrp::fillPar( IOPar& par ) const
{
    if ( true )
	mErrRet("Coming soon...");
}


bool uiGMTCoastlineGrp::usePar( const IOPar& par )
{
    return true;
}

