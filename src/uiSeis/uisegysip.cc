/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2004
 RCS:		$Id: uisegysip.cc,v 1.5 2004-07-29 21:41:26 bert Exp $
________________________________________________________________________

-*/

#include "uisegysip.h"
#include "uiseissegyentry.h"
#include "uiexecutor.h"
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
    return new uiSeisSEGYEntry( p, ioobj, 2 );
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
    uiExecutor ex( d, scanner );
    if ( !ex.go() )
	return false;

    scanner.launchBrowser( scanFile() );

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
