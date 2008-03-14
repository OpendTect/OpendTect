/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2004
 RCS:		$Id: uisegysip.cc,v 1.11 2008-03-14 11:59:11 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uisegysip.h"
#include "uiseissegyentry.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "seisscanner.h"
#include "cubesampling.h"
#include "filegen.h"
#include "ptrman.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "errh.h"


uiSEGYSurvInfoProvider::uiSEGYSurvInfoProvider( MultiID& mid )
    	: segyid(mid)
{
}


uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    PtrMan<IOObj> ioobj = IOM().get( segyid );
    return new uiSeisSEGYEntry( p, ioobj, uiSeisSEGYEntry::Scan, Seis::Vol );
}


bool uiSEGYSurvInfoProvider::getInfo( uiDialog* d, CubeSampling& cs,
				      Coord crd[3] )
{
    if ( !d ) return false;
    mDynamicCastGet(uiSeisSEGYEntry*,dlg,d)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }

    const IOObj* ioobj = dlg->ioObj();
    if ( !ioobj ) return false;

    SeisScanner scanner( *ioobj );
    uiTaskRunner ex( d );
    bool res = ex.execute( scanner );
    if ( scanner.nrDone() < 2 )
	return false;

    IOPar rep; dlg->getEntryReport( rep );
    scanner.launchBrowser( rep, scanFile() );

    if ( !scanner.getSurvInfo(cs,crd) )
    {
	uiMSG().error( scanner.message() );
	return false;
    }

    segyid = ioobj->key();
    return true;
}


const char* uiSEGYSurvInfoProvider::scanFile() const
{
    return SeisScanner::defaultUserInfoFile( "SEG-Y" );
}
