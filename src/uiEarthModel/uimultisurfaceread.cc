/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.cc,v 1.14 2008-07-17 16:14:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimultisurfaceread.h"

#include "uipossubsel.h"
#include "uilistbox.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "ioman.h"
#include "iodirentry.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "uimsg.h"


uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const char* type )
    : uiIOSurface(p,true,type)
    , singleSurfaceSelected(this)
{
    IOM().to( ctio_.ctxt.getSelKey() );
    entrylist_ = new IODirEntryList( IOM().dirPtr(), ctio_.ctxt );
    entrylist_->sort();

    BufferString lbl( "Select " ); lbl += type; lbl += "(s)";
    surfacefld_ = new uiLabeledListBox( this, lbl, true,
				       uiLabeledListBox::AboveMid );
    for ( int idx=0; idx<entrylist_->size(); idx++ )
	surfacefld_->box()->addItem( (*entrylist_)[idx]->name() );
    surfacefld_->box()->setSelected( 0 );
    surfacefld_->box()->selectionChanged.notify(
	    				mCB(this,uiMultiSurfaceRead,selCB) );
    surfacefld_->box()->doubleClicked.notify(
	    				mCB(this,uiMultiSurfaceRead,dClck) );

    mkSectionFld( true );
    sectionfld_->attach( rightTo, surfacefld_ );

    mkRangeFld();
    rgfld_->attach( leftAlignedBelow, surfacefld_ );

    if ( !strcmp(type,EMHorizon2DTranslatorGroup::keyword) ||
	 !strcmp(type,EMFaultTranslatorGroup::keyword) )
    {
	sectionfld_->display( false, true );
	rgfld_->display( false, true );
    }

    selCB(0);
}


uiMultiSurfaceRead::~uiMultiSurfaceRead()
{
    delete entrylist_;
}


void uiMultiSurfaceRead::dClck( CallBacker* )
{
    singleSurfaceSelected.trigger();
}


void uiMultiSurfaceRead::selCB( CallBacker* )
{
    const int nrsel = surfacefld_->box()->nrSelected();
    if ( nrsel > 1 )
    {
	EM::SurfaceIOData sd;
	HorSampling hs( false );
	for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
	{
	    if ( !surfacefld_->box()->isSelected(idx) ) continue;
	    const MultiID& mid = (*entrylist_)[idx]->ioobj->key();
	    const char* res = EM::EMM().getSurfaceData( mid, sd );
	    if ( res ) continue;
	    if ( hs.isEmpty() )
		hs = sd.rg;
	    else
	    {
		hs.include( sd.rg.start );
		hs.include( sd.rg.stop );
	    }
	}

	fillRangeFld( hs );
	sectionfld_->box()->empty();
	return;
    }

    for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
    {
	if ( !surfacefld_->box()->isSelected(idx) ) continue;
	fillFields( (*entrylist_)[idx]->ioobj->key() );
	break;
    }
}


void uiMultiSurfaceRead::getSurfaceIds( TypeSet<MultiID>& mids ) const
{
    mids.erase();
    for ( int idx=0; idx<surfacefld_->box()->size(); idx++ )
    {
	if ( surfacefld_->box()->isSelected(idx) )
	    mids += (*entrylist_)[idx]->ioobj->key();
    }
}


void uiMultiSurfaceRead::getSurfaceSelection( 
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );
}
