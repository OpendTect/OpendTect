/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurfacesel.h"

#include "uilistbox.h"
#include "uilistboxfilter.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfaceiodata.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"


uiSurfaceSel::uiSurfaceSel( uiParent* p, const IOObjContext& ct )
    : uiGroup(p,"Surface Selection")
    , ctxt_(*new IOObjContext(ct))
{
    listfld_ = new uiListBox( this, "listbox", OD::ChooseAtLeastOne );
    listfld_->setHSzPol( uiObject::Wide );
    filterfld_ = new uiListBoxFilter( *listfld_ );
}


uiSurfaceSel::~uiSurfaceSel()
{
    delete filterfld_;
    delete &ctxt_;
}


void uiSurfaceSel::getFullList()
{
    const IODir iodir( ctxt_.getSelKey() );
    const IODirEntryList del( iodir, ctxt_ );

    names_.erase();
    mids_.erase();
    listfld_->setEmpty();
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj || !ioobj->implExists(true) ) continue;

	mids_ += ioobj->key();
	names_.add( ioobj->name() );
    }

    filterfld_->setItems( names_ );
    listfld_->resizeToContents();
}


void uiSurfaceSel::removeFromList( const TypeSet<MultiID>& ids )
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const int surfidx = mids_.indexOf( ids[idx] );
	if ( surfidx < 0 )
	    continue;

	mids_.removeSingle( surfidx );
	filterfld_->removeItem( surfidx );
    }
}


void uiSurfaceSel::getChosen( TypeSet<MultiID>& mids ) const
{
    TypeSet<int> selidxs;
    filterfld_->getChosen( selidxs );
    for (  int idx=0; idx<selidxs.size(); idx++ )
	mids += mids_[ selidxs[idx] ];
}


void uiSurfaceSel::setChosen( const TypeSet<MultiID>& mids )
{
    BufferStringSet names;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	const MultiID mid = mids[idx];
	const int surfidx = mids_.indexOf( mid );
	if ( surfidx < 0 )
	    continue;

	names.add( IOM().nameOf(mid) );
    }

    listfld_->setChosen( names );
}


int uiSurfaceSel::nrChosen() const
{
    return filterfld_->nrChosen();
}


void uiSurfaceSel::clearList()
{
    filterfld_->setEmpty();
    names_.erase();
    mids_.erase();
}


// Deprecated
void uiSurfaceSel::getSelSurfaceIds( TypeSet<MultiID>& mids ) const
{ getChosen( mids ); }

void uiSurfaceSel::setSelSurfaceIds( const TypeSet<MultiID>& mids )
{ setChosen( mids ); }

int uiSurfaceSel::getSelItems() const
{ return nrChosen(); }


// uiSurface3DSel
uiSurface3DSel::uiSurface3DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
{}


uiSurface3DSel::~uiSurface3DSel()
{}


// uiSurface2DSel
uiSurface2DSel::uiSurface2DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
{
    getFullList();
}


uiSurface2DSel::~uiSurface2DSel()
{}


// uiHorizon2DSel
uiHorizon2DSel::uiHorizon2DSel( uiParent* p )
    : uiSurface2DSel(p,EMHorizon2DTranslatorGroup::ioContext())
{}


uiHorizon2DSel::~uiHorizon2DSel()
{}


// uiHorizon3DSel
uiHorizon3DSel::uiHorizon3DSel( uiParent* p )
    : uiSurface3DSel(p,EMHorizon3DTranslatorGroup::ioContext())
{
    getFullList();
}


uiHorizon3DSel::~uiHorizon3DSel()
{}
