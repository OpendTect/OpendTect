/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.cc,v 1.13 2004-05-06 11:16:47 bert Exp $
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
#include "uiwelldlgs.h"
#include "ptrman.h"


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
}


bool uiWellPartServer::importWell()
{
    uiWellImportAsc dlg( appserv().parent() );
    return dlg.go();
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


void uiWellPartServer::selectLogs( const MultiID& wellid, int& selidx, 
				   int& lognr )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) return;
    
    uiLogSelDlg dlg( appserv().parent(), wd->logs() );
    if ( !dlg.go() ) return;

    selidx = dlg.selectedLog();
    lognr = dlg.logNumber();
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( appserv().parent() );
    dlg.go();
}
