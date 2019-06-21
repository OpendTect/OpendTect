/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uisurfacesel.h"

#include "uilistbox.h"

#include "ioobjctxt.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfaceiodata.h"
#include "dbdir.h"
#include "ioobj.h"


uiSurfaceSel::uiSurfaceSel( uiParent* p, const IOObjContext& ct )
    : uiGroup(p,"Surface Selection")
    , ctxt_(*new IOObjContext(ct))
{
    listfld_ = new uiListBox( this, "listbox", OD::ChooseAtLeastOne );
    listfld_->setHSzPol( uiObject::Wide );
}


uiSurfaceSel::~uiSurfaceSel()
{
    delete &ctxt_;
}


void uiSurfaceSel::getFullList()
{
    names_.erase();
    mids_.erase();
    listfld_->setEmpty();

    const DBDirEntryList del( ctxt_ );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj& ioobj = del.ioobj( idx );
	if ( ioobj.implExists(true) )
	{
	    mids_ += ioobj.key();
	    names_.add( del.dispName(idx) );
	}
    }

    listfld_->addItems( names_ );
}


void uiSurfaceSel::removeFromList( const DBKeySet& ids )
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const int surfidx = mids_.indexOf( ids[idx] );
	if ( surfidx < 0 )
	    continue;

	listfld_->removeItem( surfidx );
	mids_.removeSingle( surfidx );
    }
}


void uiSurfaceSel::getSelSurfaceIds( DBKeySet& mids ) const
{
    TypeSet<int> selidxs;
    listfld_->getChosen( selidxs );
    for (  int idx=0; idx<selidxs.size(); idx++ )
	mids += mids_[ selidxs[idx] ];
}


void uiSurfaceSel::setSelSurfaceIds( const DBKeySet& mids )
{
    TypeSet<int> selidxs;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const int surfidx = mids_.indexOf( mids[idx] );
	if ( surfidx < 0 ) continue;
	selidxs += surfidx;
    }

     listfld_->setChosen( selidxs );
}


int uiSurfaceSel::getSelItems() const
{ return listfld_->nrChosen(); }


void uiSurfaceSel::clearList()
{
    listfld_->setEmpty();
    names_.erase();
    mids_.erase();
}


uiSurface3DSel::uiSurface3DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
{}



uiSurface2DSel::uiSurface2DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
{
    getFullList();
}


uiHorizon2DSel::uiHorizon2DSel( uiParent* p )
    : uiSurface2DSel(p,EMHorizon2DTranslatorGroup::ioContext())
{}



uiHorizon3DSel::uiHorizon3DSel( uiParent* p )
    : uiSurface3DSel(p,EMHorizon3DTranslatorGroup::ioContext())
{
    getFullList();
}
