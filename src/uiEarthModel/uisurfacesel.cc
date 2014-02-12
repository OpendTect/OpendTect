/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurfacesel.h"

#include "uilistbox.h"

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
    listfld_ = new uiListBox( this, "listbox", true );
    listfld_->setHSzPol( uiObject::Wide );
}


uiSurfaceSel::~uiSurfaceSel()
{
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
	if ( !ioobj ) continue;

	mids_ += ioobj->key();
	names_.add( ioobj->name() );
    }
    
    listfld_->addItems( names_ );
}


void uiSurfaceSel::removeFromList( const TypeSet<MultiID>& ids )
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


void uiSurfaceSel::getSelSurfaceIds( TypeSet<MultiID>& mids ) const
{
    TypeSet<int> selidxs;
    listfld_->getSelectedItems( selidxs );
    for (  int idx=0; idx<selidxs.size(); idx++ )
	mids += mids_[ selidxs[idx] ];
}


int uiSurfaceSel::getSelItems() const
{ return listfld_->nrSelected(); }


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
    , linesetid_(-1)
{
    getFullList();
}


void uiSurface2DSel::setLineSetID( const MultiID& mid )
{
    linesetid_ = mid;
    
    const IODir iodir( ctxt_.getSelKey() );
    const IODirEntryList del( iodir, ctxt_ );

    names_.erase();
    mids_.erase();
    listfld_->setEmpty();
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj ) continue;

	EM::IOObjInfo eminfo( ioobj->key() );
	BufferStringSet linesets;
	eminfo.getLineSets( linesets );
	for ( int idz=0; idz<linesets.size(); idz++ )
	{
	    IOObj* selobj = IOM().get( mid );
	    if ( selobj && linesets.get(idz) == selobj->name() )
	    {
		mids_.addIfNew( ioobj->key() );
		names_.addIfNew( ioobj->name() );
	    }
	}
    }
    listfld_->addItems( names_ );
}


uiHorizon2DSel::uiHorizon2DSel( uiParent* p )
    : uiSurface2DSel(p,EMHorizon2DTranslatorGroup::ioContext())
{}



uiHorizon3DSel::uiHorizon3DSel( uiParent* p )
    : uiSurface3DSel(p,EMHorizon3DTranslatorGroup::ioContext())
{
    getFullList();
}
