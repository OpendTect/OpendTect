/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		January 2015
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id $";

#include "uibasemapiomgr.h"

#include "basemaptr.h"
#include "ioman.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"

uiBasemapIOMgr::uiBasemapIOMgr( uiParent* parent )
    : parent_(parent)
    , curbasemapid_(MultiID::udf())
{
}


uiBasemapIOMgr::~uiBasemapIOMgr()
{
}


bool uiBasemapIOMgr::read( bool haschanged )
{
    CtxtIOObj ctio( mIOObjContext(Basemap) );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( parent_, ctio );
    if ( !dlg.go() ) return false;

    if ( curbasemapid_ == dlg.chosenID() && !haschanged )
    {
	if ( !uiMSG().askContinue(
		 tr("Are you trying to reload the same Basemap?")) )
	    return false;
    }
    else if ( !curbasemapid_.isUdf() && haschanged )
    {
	if ( !uiMSG().askContinue(
		 tr("Any unsaved changes will be lost. Are you sure "
		     "you want to continue?")) )
	    return false;
    }

    curbasemapid_ = dlg.chosenID();

    BufferString errmsg; IOPar itmpars;
    if ( !BasemapTranslator::retrieve(itmpars,dlg.ioObj(),errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }
    else
    {
	BMM().removeAllItems();
	BMM().addfromPar( itmpars );
    }
    return true;
}


bool uiBasemapIOMgr::save( bool saveas )
{
    const ObjectSet<uiBasemapTreeItem>& treeitms = BMM().treeitems();
    int nrtreeitems = treeitms.size();
    IOPar itmpars;
    itmpars.set( sKey::NrItems(), nrtreeitems );

    for ( int idx=0; idx<nrtreeitems; idx++ )
	itmpars.mergeComp( treeitms[idx]->pars(), toString(idx) );

    if ( saveas )
    {
	CtxtIOObj ctio( mIOObjContext(Basemap) );
	ctio.ctxt.forread = false;
	uiIOObjSelDlg dlg( parent_, ctio );
	if ( !dlg.go() ) return false;

	curbasemapid_ = dlg.chosenID();
    }

    PtrMan<IOObj> ioobj = IOM().get( curbasemapid_ );
    BufferString errmsg;
    if ( !BasemapTranslator::store(itmpars,ioobj,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}
