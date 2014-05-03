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
#include "uimsg.h"

#include "ctxtioobj.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"


uiMultiSurfaceReadDlg::uiMultiSurfaceReadDlg( uiParent* p, const char* type )
    : uiDialog(p,uiDialog::Setup( BufferString( "Select Input ",type,"(s)" ),
				  mNoDlgTitle,
                                  mODHelpKey(mMultiSurfaceReadDlgHelpID) )
                                  .nrstatusflds(1) )
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
uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const char* typ )
    : uiIOSurface(p,true,typ)
    , singleSurfaceSelected(this)
{
    ioobjselgrp_ = new uiIOObjSelGrp( this, ctio_->ctxt,
			uiIOObjSelGrp::Setup(uiIOObjSelGrp::AtLeastOne) );
    ioobjselgrp_->selectionChg.notify( mCB(this,uiMultiSurfaceRead,selCB) );
    ioobjselgrp_->getListField()->doubleClicked.notify(
					mCB(this,uiMultiSurfaceRead,dClck) );

    mkRangeFld();
    rgfld_->attach( leftAlignedBelow, ioobjselgrp_ );

    const FixedString type( typ );
    if ( type == EMHorizon2DTranslatorGroup::keyword() ||
	 type == EMFaultStickSetTranslatorGroup::keyword() ||
         type == EMFault3DTranslatorGroup::keyword() )
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

    const int nrsel = ioobjselgrp_->nrSelected();
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
    const int nrsel = ioobjselgrp_->nrSelected();
    BufferString errormsgstr;
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID mid = ioobjselgrp_->selected( idx );
	const EM::IOObjInfo info( mid );
	EM::SurfaceIOData sd;
	BufferString errmsg;
	if ( info.getSurfaceData(sd,errmsg) )
	    mids += mid;
	else
	{
	    if ( !info.ioObj() )
		continue;

	    errormsgstr += info.ioObj()->name();
	    errormsgstr += " :  ";
	    errormsgstr += errmsg;
	    errormsgstr += "\n";
	}

    }

    if ( !errormsgstr.isEmpty() )
    {
	if ( nrsel == 1  )
	    uiMSG().error( errormsgstr );
	else
	    uiMSG().error( "The following selections will not be loaded \n\n",
			    errormsgstr );
    }

}


void uiMultiSurfaceRead::getSurfaceSelection(
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );

    if ( ioobjselgrp_->nrSelected() != 1 )
	return;

    const MultiID mid = ioobjselgrp_->selected( 0 );
    const EM::IOObjInfo info( mid );
    EM::SurfaceIOData sd;
    BufferString errmsg;
    if ( !info.getSurfaceData(sd,errmsg) || sd.sections.size() < 2
	    || !info.isHorizon() )
	return;

    uiDialog dlg( const_cast<uiParent*>(parent()),
	    uiDialog::Setup("Select section(s)",mNoDlgTitle,mNoHelpKey) );
    uiListBox* lb = new uiListBox( &dlg, "Patches", true );
    lb->addItems( sd.sections );
    lb->selectAll( true );
    if ( dlg.go() )
    {
	sel.selsections.erase();
	lb->getSelectedItems( sel.selsections );
	if ( sel.selsections.isEmpty() )
	    sel.selsections += 0;
    }
}
