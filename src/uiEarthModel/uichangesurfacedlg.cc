/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra / Bert Bril
 Date:		Sep 2005 / Nov 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uichangesurfacedlg.cc,v 1.33 2009-11-04 16:01:05 cvsyuancheng Exp $";

#include "uichangesurfacedlg.h"

#include "uiarray2dchg.h"
#include "uitaskrunner.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "undo.h"

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"


uiChangeSurfaceDlg::uiChangeSurfaceDlg( uiParent* p, EM::Horizon3D* hor,
					const char* txt )
    : uiDialog (p, Setup(txt,mNoDlgTitle,"104.0.3") )
    , horizon_( hor )
    , savefldgrp_( 0 )		   
    , inputfld_( 0 )
    , parsgrp_( 0 )
{
    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread = true;
	inputfld_ = new uiIOObjSel( this, ctxt, "Input Horizon" );
    }

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_ );
    savefldgrp_->setSaveFieldName( "Save interpolated horizon" );
}

void uiChangeSurfaceDlg::attachPars()
{
    if ( !parsgrp_ ) return;

    if ( inputfld_ )
	parsgrp_->attach( alignedBelow, inputfld_ );
    
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, parsgrp_ );

    savefldgrp_->attach( alignedBelow, parsgrp_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiChangeSurfaceDlg::~uiChangeSurfaceDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiChangeSurfaceDlg::readHorizon()
{
    if ( !inputfld_->ctxtIOObj().ioobj )
	return false;

    const MultiID& mid = inputfld_->ctxtIOObj().ioobj->key();
    EM::Horizon3D* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) return false;

    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();

    return true;
}


bool uiChangeSurfaceDlg::doProcessing()
{
    MouseCursorChanger chgr( MouseCursor::Wait );
    bool change = false;
    EM::Horizon3D* usedhor = savefldgrp_->getNewHorizon() ?
       savefldgrp_->getNewHorizon() : horizon_;

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	if ( !idx && needsFullSurveyArray() )
	    savefldgrp_->setFullSurveyArray( true );

	PtrMan<Array2D<float> > arr = horizon_->createArray2D( sid );
	if ( !arr )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	PtrMan<Executor> worker = getWorker( *arr,
			horizon_->geometry().rowRange(sid),
			horizon_->geometry().colRange(sid) );
	if ( !worker ) return false;

	uiTaskRunner dlg( this );
	if ( !dlg.execute(*worker) )
	    return false;

	const EM::SectionID usedsid = usedhor->geometry().sectionID( idx );
	if ( !usedhor->setArray2D(*arr, usedsid, fillUdfsOnly(), undoText()) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += usedsid;
	    ErrMsg( msg ); continue;
        }
	else if ( usedhor==horizon_ )
	{
	    change = true;
	}
    }

    if ( change )
	EM::EMM().undo().setUserInteractionEnd(EM::EMM().undo().lastEventID());

    return true;
}


bool uiChangeSurfaceDlg::acceptOK( CallBacker* cb )
{
    if ( inputfld_ && !inputfld_->commitInput() )
	mErrRet( "Please select input horizon" )

    if ( !horizon_ && !readHorizon() )
	mErrRet( "Cannot read horizon" )
   
    if ( !savefldgrp_->acceptOK( cb ) )
	return false;

    if ( !doProcessing() )
	return false;

    return savefldgrp_->saveHorizon();
}


//---- uiFilterHorizonDlg

uiFilterHorizonDlg::uiFilterHorizonDlg( uiParent* p, EM::Horizon3D* hor )
    : uiChangeSurfaceDlg(p,hor,"Horizon filtering")
{
    parsgrp_ = new uiArr2DFilterPars( this );
    attachPars();
}


Executor* uiFilterHorizonDlg::getWorker( Array2D<float>& a2d,
					   const StepInterval<int>& rowrg,
					   const StepInterval<int>& colrg )
{
    Array2DFilterPars pars = ((uiArr2DFilterPars*)parsgrp_)->getInput();
    return new Array2DFilterer<float>( a2d, pars );
}
