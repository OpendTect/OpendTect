/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.7 2002-06-19 15:41:30 bert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"
#include "uimergeseis.h"
#include "uiseissegyimpexp.h"
#include "uiseisidealimpexp.h"
#include "uiseiscbvsimp.h"
#include "uiseisfileman.h"
#include "uimsg.h"
#include "idealconn.h"
#include "ptrman.h"


bool uiSeisPartServer::isAvailable( uiSeisPartServer::ExternalType t ) const
{
    return t == uiSeisPartServer::SegY || t == uiSeisPartServer::CBVS
	|| (int)t == ((int)IdealConn::guessedType() + 1);
}


bool uiSeisPartServer::ioSeis( uiSeisPartServer::ExternalType t, bool forread )
{
#ifndef sun5
    if ( t != uiSeisPartServer::SegY && t != uiSeisPartServer::CBVS
      && !uiMSG().askGoOn( "Sorry, workstation connection not available on "
			    "this platform.\nPlease import from a "
			    "Solaris workstation or use SEG-Y.\n\n"
			    "Do you wish to see the dialog anyway?" ) )
	return false;
#endif

    PtrMan<uiDialog> dlg = t == uiSeisPartServer::SegY
      ?	(uiDialog*)new uiSeisSegYImpExp( appserv().parent(), forread, segyid )
      : ( t == uiSeisPartServer::CBVS ?
	(uiDialog*)new uiSeisImpCBVS( appserv().parent() )
      : (uiDialog*)new uiSeisIdealImpExp( appserv().parent(), forread,
	      				  (int)t==1 ) );

    return dlg->go();
}


bool uiSeisPartServer::importSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, true ); }
bool uiSeisPartServer::exportSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, false ); }


bool uiSeisPartServer::mergeSeis()
{
    uiMergeSeis dlg( appserv().parent() );
    return dlg.go();
}


void uiSeisPartServer::manageSeismics()
{
    uiSeisFileMan dlg( appserv().parent() );
    dlg.go();
}
