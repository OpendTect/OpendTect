/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.cc,v 1.1 2003-08-29 07:25:01 nanne Exp $
________________________________________________________________________

-*/


#include "uiwellpartserv.h"
#include "uiwellimpasc.h"
#include "welltransl.h"
#include "multiid.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "idealconn.h"
#include "ptrman.h"


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
}


bool uiWellPartServer::importWell( uiWellPartServer::ExternalType t )
{ return ioWell( t, true ); }

bool uiWellPartServer::exportWell( uiWellPartServer::ExternalType t )
{ return ioWell( t, false ); }


bool uiWellPartServer::ioWell( uiWellPartServer::ExternalType t, bool imp )
{
    if ( t != Ascii && !IdealConn::haveIdealServices() &&
	 !uiMSG().askGoOn( "No idealconn, see dialog anyway?" ) )
	return false;

    PtrMan<uiDialog> dlg = 0;
    if ( t == Ascii && imp )
	dlg = (uiDialog*)new uiWellImportAsc( appserv().parent() );

    return dlg && dlg->go();
}


bool uiWellPartServer::selectWells( ObjectSet<MultiID>& wellids )
{
    CtxtIOObj* ctio = new CtxtIOObj( WellTranslator::ioContext() );
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( !dlg.go() ) return false;

    deepErase( wellids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	wellids += new MultiID( dlg.selected(idx)->key() );

    return wellids.size();
}


void uiWellPartServer::manageWells()
{
}
