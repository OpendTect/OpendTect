/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurfacesel.cc,v 1.1 2009-04-01 11:55:47 cvsnanne Exp $";

#include "uisurfacesel.h"

#include "uilistbox.h"

#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"


uiSurfaceSel::uiSurfaceSel( uiParent* p, const IOObjContext& ct )
    : uiGroup(p,"Surface Selection")
    , ctxt_(*new IOObjContext(ct))
{
    listfld_ = new uiLabeledListBox( this, "", true );
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
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( !ioobj ) continue;

	mids_ += ioobj->key();
	names_.add( ioobj->name() );
    }
}


void uiSurfaceSel::getSelSurfaceIds( TypeSet<MultiID>& mids ) const
{
    TypeSet<int> selidxs;
    listfld_->box()->getSelectedItems( selidxs );
    for (  int idx=0; idx<selidxs.size(); idx++ )
	mids += mids_[ selidxs[idx] ];
}



uiSurface3DSel::uiSurface3DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
{}



uiSurface2DSel::uiSurface2DSel( uiParent* p, const IOObjContext& ct )
    : uiSurfaceSel( p, ct )
    , linesetid_(-1)
{}


void uiSurface2DSel::setLineSetID( const MultiID& mid )
{ linesetid_ = mid; }


void uiSurface2DSel::getSelSurfaceIds( TypeSet<MultiID>& mids ) const
{
}



uiHorizon2DSel::uiHorizon2DSel( uiParent* p )
    : uiSurface2DSel(p,EMHorizon2DTranslatorGroup::ioContext())
{}
