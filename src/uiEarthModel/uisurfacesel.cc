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


uiSurfaceSelGrp::uiSurfaceSelGrp( uiParent* p, const IOObjContext& ct )
    : uiGroup(p,"Surface Selection")
    , ctxt_(*new IOObjContext(ct))
{
    listfld_ = new uiListBox( this, "listbox", OD::ChooseAtLeastOne );
    listfld_->setHSzPol( uiObject::Wide );
    filterfld_ = new uiListBoxFilter( *listfld_ );
}


uiSurfaceSelGrp::~uiSurfaceSelGrp()
{
    delete filterfld_;
    delete &ctxt_;
}


void uiSurfaceSelGrp::getFullList()
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


void uiSurfaceSelGrp::removeFromList( const TypeSet<MultiID>& ids )
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


void uiSurfaceSelGrp::getChosen( TypeSet<MultiID>& mids ) const
{
    TypeSet<int> selidxs;
    filterfld_->getChosen( selidxs );
    for (  int idx=0; idx<selidxs.size(); idx++ )
	mids += mids_[ selidxs[idx] ];
}


void uiSurfaceSelGrp::setChosen( const TypeSet<MultiID>& mids )
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


int uiSurfaceSelGrp::nrChosen() const
{
    return filterfld_->nrChosen();
}


void uiSurfaceSelGrp::clearList()
{
    filterfld_->setEmpty();
    names_.erase();
    mids_.erase();
}


// Deprecated
void uiSurfaceSelGrp::getSelSurfaceIds( TypeSet<MultiID>& mids ) const
{ getChosen( mids ); }

void uiSurfaceSelGrp::setSelSurfaceIds( const TypeSet<MultiID>& mids )
{ setChosen( mids ); }

int uiSurfaceSelGrp::getSelItems() const
{ return nrChosen(); }


// uiSurface3DSelGrp
uiSurface3DSelGrp::uiSurface3DSelGrp( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSelGrp( p, ct )
{}


uiSurface3DSelGrp::~uiSurface3DSelGrp()
{}


// uiSurface2DSelGrp
uiSurface2DSelGrp::uiSurface2DSelGrp( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSelGrp( p, ct )
{
    getFullList();
}


uiSurface2DSelGrp::~uiSurface2DSelGrp()
{}


// uiHorizon2DSelGrp
uiHorizon2DSelGrp::uiHorizon2DSelGrp( uiParent* p )
    : uiSurface2DSelGrp(p,EM::Horizon::ioContext(true,true))
{}


uiHorizon2DSelGrp::~uiHorizon2DSelGrp()
{}


// uiHorizon3DSelGrp
uiHorizon3DSelGrp::uiHorizon3DSelGrp( uiParent* p )
    : uiSurface3DSelGrp(p,EM::Horizon::ioContext(false,true))
{
    getFullList();
}


uiHorizon3DSelGrp::~uiHorizon3DSelGrp()
{}
