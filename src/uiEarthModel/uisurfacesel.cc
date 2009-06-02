/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurfacesel.cc,v 1.4 2009-06-02 13:38:54 cvsnanne Exp $";

#include "uisurfacesel.h"

#include "uilistbox.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfaceiodata.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"


uiSurfaceSel::uiSurfaceSel( uiParent* p, const IOObjContext& ct )
    : uiGroup(p,"Surface Selection")
    , ctxt_(*new IOObjContext(ct))
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "", true );
    listfld_ = llb->box();
    listfld_->setHSzPol( uiObject::Wide );
}


uiSurfaceSel::~uiSurfaceSel()
{
    delete &ctxt_;
}


void uiSurfaceSel::getFullList()
{
    IOM().to( 0 ); IOM().to( ctxt_.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctxt_ );

    names_.erase();
    mids_.erase();
    listfld_->clear();
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( !ioobj ) continue;

	mids_ += ioobj->key();
	names_.add( ioobj->name() );
    }
    
    listfld_->addItems( names_ );
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
    
    IOM().to( 0 ); IOM().to( ctxt_.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctxt_ );

    names_.erase();
    mids_.erase();
    listfld_->clear();
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( !ioobj ) continue;

	EM::SurfaceIOData sd;
	EM::EMM().getSurfaceData( ioobj->key(), sd );
	for ( int idz=0; idz<sd.linesets.size(); idz++ )
	{
	    IOObj* selobj = IOM().get( mid );
	    if ( *sd.linesets[idz] == selobj->name() )
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
