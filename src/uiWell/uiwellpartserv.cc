/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.cc,v 1.17 2005-10-24 15:17:25 cvshelene Exp $
________________________________________________________________________

-*/


#include "uiwellpartserv.h"
#include "uiwellimpasc.h"
#include "uiwellman.h"
#include "welltransl.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welltrack.h"
#include "welllogset.h"
#include "uiwellrdmlinedlg.h"
#include "multiid.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiwelldlgs.h"
#include "ptrman.h"


const int uiWellPartServer::evPreviewRdmLine			=0;


uiWellPartServer::uiWellPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
}


bool uiWellPartServer::importTrack()
{
    uiWellImportAsc dlg( appserv().parent() );
    return dlg.go();
}


bool uiWellPartServer::importLogs()
{
    manageWells(); return true;
}


bool uiWellPartServer::importMarkers()
{
    manageWells(); return true;
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


bool uiWellPartServer::hasLogs( const MultiID& wellid ) const
{
    const Well::Data* wd = Well::MGR().get( wellid );
    return wd && wd->logs().size();
}


void uiWellPartServer::manageWells()
{
    uiWellMan dlg( appserv().parent() );
    dlg.go();
}


void uiWellPartServer::selectWellCoordsForRdmLine()
{
    uiWell2RandomLineDlg dlg( appserv().parent(), this );
    dlg.go();
}


const char* uiWellPartServer::askWellName()
{
    uiWellNameDlg dlg( appserv().parent() );
    dlg.go();
    return dlg.wellname;
}


#define mErrRet(s) { errmsg = s; return false; }


bool uiWellPartServer::storeWell( const TypeSet<Coord3>& newcoords, 
				  const char* errmsg )// TODO:name
{
    Well::Track welltrack("helenewell");
    for ( int idx=0; idx<newcoords.size(); idx++ )
    {
	welltrack.addPoint( Coord(newcoords[idx].x,newcoords[idx].y), 
			    newcoords[idx].z, 0 );
    }
    Well::Data welldata("helenewell");
    welldata.track() = welltrack;
    CtxtIOObj ctio(*mMkCtxtIOObj(Well));
    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
			 "Another type object with this name already exists." );

    if ( !wtr->write(welldata,*ctio.ioobj) ) mErrRet( "Cannot write well" );

    return true;

    //TODO

}
