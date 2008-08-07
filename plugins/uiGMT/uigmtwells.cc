/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtwells.cc,v 1.1 2008-08-07 12:38:39 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtwells.h"

#include "gmtpar.h"
#include "uimsg.h"


int uiGMTWellsGrp::factoryid_ = -1;

void uiGMTWellsGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Wells",
				    uiGMTWellsGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTWellsGrp::createInstance( uiParent* p )
{
    return new uiGMTWellsGrp( p );
}


uiGMTWellsGrp::uiGMTWellsGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Wells")
{
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTWellsGrp::fillPar( IOPar& par ) const
{
    if ( true )
	mErrRet("Coming soon...");
}


bool uiGMTWellsGrp::usePar( const IOPar& par )
{
    return true;
}

