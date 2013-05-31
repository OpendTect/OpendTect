/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimultisurfaceread.h"

#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uipossubsel.h"

#include "ctxtioobj.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"


uiMultiSurfaceReadDlg::uiMultiSurfaceReadDlg( uiParent* p, const char* type ) 
    : uiDialog(p,uiDialog::Setup( BufferString( type," selection" ),
				  BufferString( "Select Input ",type,"(s)" ),
				  "104.3.0").nrstatusflds(1) )
{
    surfacefld_ = new uiMultiSurfaceRead( this, type );
    surfacefld_->objselGrp()->newStatusMsg.notify( 
				mCB(this,uiMultiSurfaceReadDlg,statusMsg) );
    surfacefld_->singleSurfaceSelected.notify( mCB(this,uiDialog,accept) );
}


void uiMultiSurfaceReadDlg::statusMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(const char*,msg,cb);
    toStatusBar( msg );
}


bool uiMultiSurfaceReadDlg::acceptOK( CallBacker* )
{
    return surfacefld_->objselGrp()->processInput();
}


// ***** uiMultiSurfaceRead *****
uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const char* type )
    : uiIOSurface(p,true,type)
    , singleSurfaceSelected(this)
{
    ioobjselgrp_ = new uiIOObjSelGrp( this, ctio_->ctxt, "", true );
    ioobjselgrp_->selectionChg.notify( mCB(this,uiMultiSurfaceRead,selCB) );
    ioobjselgrp_->getListField()->doubleClicked.notify( 
					mCB(this,uiMultiSurfaceRead,dClck) );

    mkRangeFld();
    rgfld_->attach( leftAlignedBelow, ioobjselgrp_ );

    if ( !strcmp(type,EMHorizon2DTranslatorGroup::keyword()) ||
	 !strcmp(type,EMFaultStickSetTranslatorGroup::keyword()) ||
         !strcmp(type,EMFault3DTranslatorGroup::keyword()) )
    {
	rgfld_->display( false, true );
    }

    selCB(0);
}


uiMultiSurfaceRead::~uiMultiSurfaceRead()
{
}


void uiMultiSurfaceRead::dClck( CallBacker* )
{
    singleSurfaceSelected.trigger();
}


void uiMultiSurfaceRead::selCB( CallBacker* cb )
{
    if ( !rgfld_->mainObject() || !rgfld_->mainObject()->isDisplayed() ) return;

    const int nrsel = ioobjselgrp_->nrSel();
    if( nrsel == 0 )
	return;

    if ( nrsel > 1 )
    {
	EM::SurfaceIOData sd;
	HorSampling hs( false );
	if ( !processInput() ) return;
	for ( int idx=0; idx<nrsel; idx++ )
	{
	    const MultiID& mid = ioobjselgrp_->selected( idx );

	    EM::IOObjInfo eminfo( mid );
	    if ( !eminfo.isOK() ) continue;
	    HorSampling emhs;
	    emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );

	    if ( hs.isEmpty() )
		hs = emhs;
	    else if ( !emhs.isEmpty() )
		hs.include( emhs, false );
	}

	fillRangeFld( hs );
	return;
    }

    if ( processInput() ) 
    {
	if ( !cb )
	{
	    if ( !fillFields(ioobjselgrp_->selected(0), false) )
		ioobjselgrp_->getListField()->clear();
	}
	else
	    fillFields( ioobjselgrp_->selected(0) );
    }
}


void uiMultiSurfaceRead::getSurfaceIds( TypeSet<MultiID>& mids ) const
{
    mids.erase();
    const int nrsel = ioobjselgrp_->nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
	mids += ioobjselgrp_->selected( idx );
}


void uiMultiSurfaceRead::getSurfaceSelection( 
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );
}
