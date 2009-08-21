/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra / Bert Bril
 Date:		Sep 2005 / Nov 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uichangesurfacedlg.cc,v 1.32 2009-08-21 10:11:46 cvsbert Exp $";

#include "uichangesurfacedlg.h"

#include "uiarray2dchg.h"
#include "mousecursor.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uistepoutsel.h"
#include "uimsg.h"
#include "undo.h"

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "errh.h"
#include "survinfo.h"


uiChangeSurfaceDlg::uiChangeSurfaceDlg( uiParent* p, EM::Horizon3D* hor,
					const char* txt )
    : uiDialog(p,Setup(txt,mNoDlgTitle,"104.0.3"))
    , horizon_(hor)
    , inputfld_(0)
    , outputfld_(0)
    , savefld_(0)
    , ctioin_(0)
    , ctioout_(0)
    , parsgrp_(0)
{
    if ( !horizon_ )
    {
	ctioin_ = mMkCtxtIOObj( EMHorizon3D );
	ctioin_->ctxt.forread = true;
	inputfld_ = new uiIOObjSel( this, *ctioin_, "Input Horizon" );
    }

    savefld_ = new uiGenInput( this, "Save interpolated horizon",
			       BoolInpSpec(true,"As new","Overwrite") );
    savefld_->valuechanged.notify( mCB(this,uiChangeSurfaceDlg,saveCB) );

    ctioout_ = mMkCtxtIOObj( EMHorizon3D );
    ctioout_->ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, *ctioout_, "Output Horizon" );
    outputfld_->attach( alignedBelow, savefld_ );
    saveCB(0);

    if ( horizon_ ) horizon_->ref();
}

void uiChangeSurfaceDlg::attachPars()
{
    if ( !parsgrp_ ) return;

    if ( inputfld_ )
	parsgrp_->attach( alignedBelow, inputfld_ );
    if ( savefld_ )
	savefld_->attach( alignedBelow, parsgrp_ );
}


uiChangeSurfaceDlg::~uiChangeSurfaceDlg()
{
    if ( ctioin_ ) { delete ctioin_->ioobj; delete ctioin_; }
    if ( ctioout_ ) { delete ctioout_->ioobj; delete ctioout_; }
    if ( horizon_ ) horizon_->unRef();
}


void uiChangeSurfaceDlg::saveCB( CallBacker* )
{
    outputfld_->display( savefld_->getBoolValue() );
}


bool uiChangeSurfaceDlg::readHorizon()
{
    const MultiID& mid = inputfld_->ctxtIOObj().ioobj->key();
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader ) return false;

	uiTaskRunner dlg( this );
	if ( !dlg.execute(*reader) )
	{
	    delete reader;
	    return false;
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return true;
}


bool uiChangeSurfaceDlg::doProcessing()
{
    MouseCursorChanger chgr( MouseCursor::Wait );
    bool change = false;
    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	StepInterval<int> rowrg = horizon_->geometry().rowRange( sid );
	StepInterval<int> colrg = horizon_->geometry().colRange( sid );

	mDynamicCastGet( Geometry::ParametricSurface*, surf,
			 horizon_->sectionGeometry( sid ) );

	if ( !idx && surf && needsFullSurveyArray() )
	{
	    const StepInterval<int> survcrlrg = SI().crlRange(true);
	    while ( colrg.start-colrg.step>=survcrlrg.start )
	    {
		const int newcol = colrg.start-colrg.step;
		surf->insertCol( newcol );
		colrg.start = newcol;
	    }

	    while ( colrg.stop+colrg.step<=survcrlrg.stop )
	    {
		const int newcol = colrg.stop+colrg.step;
		surf->insertCol( newcol );
		colrg.stop = newcol;
	    }

	    const StepInterval<int> survinlrg = SI().inlRange(true);
	    while ( rowrg.start-rowrg.step>=survinlrg.start )
	    {
		const int newrow = rowrg.start-rowrg.step;
		surf->insertRow( newrow );
		rowrg.start = newrow;
	    }

	    while ( rowrg.stop+rowrg.step<=survinlrg.stop )
	    {
		const int newrow = rowrg.stop+rowrg.step;
		surf->insertRow( newrow );
		rowrg.stop = newrow;
	    }
	}

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

	if ( !savefld_ )
	{
	    BufferString msg = infoMsg( worker );
	    if ( !msg.isEmpty() )
		uiMSG().message( msg );
	}

	if ( !horizon_->setArray2D( *arr, sid, fillUdfsOnly(), undoText() ) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
        }
	else
	{
	    change = true;
	}
    }

    if ( change )
	EM::EMM().undo().setUserInteractionEnd(EM::EMM().undo().lastEventID());


    return true;
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiChangeSurfaceDlg::saveHorizon()
{
    PtrMan<Executor> exec = 0;
    const bool saveas = savefld_ && savefld_->getBoolValue();
    if ( !saveas )
	exec = horizon_->saver();
    else
    {
	const MultiID& mid = outputfld_->ctxtIOObj().ioobj->key();
	horizon_->setMultiID( mid );
	exec = horizon_->geometry().saver( 0, &mid );
    }

    if ( !exec )
    {
	uiMSG().error( "Cannot save horizon" );
	return false;
    }

    uiTaskRunner dlg( this );
    const bool res = dlg.execute( *exec );
    return res;
}


bool uiChangeSurfaceDlg::acceptOK( CallBacker* )
{
    if ( inputfld_ && !inputfld_->commitInput() )
	mErrRet( "Please select input horizon" )

    const bool savetodisk = outputfld_;
    const bool saveasnew = savefld_ && savefld_->getBoolValue();
    if ( savetodisk && savefld_->getBoolValue() && !outputfld_->commitInput() )
	mErrRet( outputfld_->isEmpty() ? "Please select output horizon" : 0 )

    if ( !horizon_ && !readHorizon() )
	mErrRet( "Cannot read horizon" )

    if ( !doProcessing() )
	return false;

    if ( savetodisk && !saveHorizon() )
	return false;

    return true;
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
