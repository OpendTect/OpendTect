/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.10 2003-10-27 23:10:02 bert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"
#include "uimergeseis.h"
#include "uiseissegyimpexp.h"
#include "uiseiscbvsimp.h"
#include "uiseisfileman.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "ptrman.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    	: uiApplPartServer(a)
{
}


bool uiSeisPartServer::ioSeis( uiSeisPartServer::ExternalType t, bool forread )
{
    PtrMan<uiDialog> dlg = t == uiSeisPartServer::SegY
      ?	(uiDialog*)new uiSeisSegYImpExp( appserv().parent(), forread, segyid )
      : (uiDialog*)new uiSeisImpCBVS( appserv().parent() );

    return dlg->go();
}


bool uiSeisPartServer::importSeis( uiSeisPartServer::ExternalType t )
{ return ioSeis( t, true ); }
bool uiSeisPartServer::exportSeis()
{ return ioSeis( uiSeisPartServer::SegY, false ); }


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
