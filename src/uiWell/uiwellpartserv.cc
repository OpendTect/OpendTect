/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.cc,v 1.4 2003-10-16 15:01:27 nanne Exp $
________________________________________________________________________

-*/


#include "uiwellpartserv.h"
#include "uiwellimpasc.h"
#include "uiwellman.h"
#include "welltransl.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "multiid.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "idealconn.h"
#include "uiwelldlgs.h"
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
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( wellids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	wellids += new MultiID( dlg.selected(idx)->key() );

    return wellids.size();
}


void uiWellPartServer::selectLogs( const MultiID& wellid, int& selidx )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) return;
    
    ObjectSet<BufferString> lognames;
    for ( int idx=0; idx<wd->logs().size(); idx++ )
	lognames += new BufferString( wd->logs().getLog(idx).name() );

    uiLogSelDlg dlg( appserv().parent(), lognames );
    if ( !dlg.go() ) return;

    selidx = dlg.selectedLog();
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( appserv().parent() );
    dlg.go();
}
