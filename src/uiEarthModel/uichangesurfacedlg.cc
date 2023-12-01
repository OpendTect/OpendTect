/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uichangesurfacedlg.h"

#include "uiarray2dchg.h"
#include "uitaskrunner.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
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
#include "od_helpids.h"


uiChangeHorizonDlg::uiChangeHorizonDlg( uiParent* p, EM::Horizon* hor,
					bool is2d, const uiString& txt )
    : uiDialog(p,Setup(txt,mNoDlgTitle,
			mODHelpKey(mChangeSurfaceDlgHelpID)))
    , horReadyForDisplay(this)
    , inputfld_(nullptr)
    , parsgrp_(nullptr)
    , horizon_(hor)
    , is2d_(is2d)
{
    setCtrlStyle( RunAndClose );

    if ( horizon_ )
	horizon_->ref();
    else
	inputfld_ = new uiHorizonSel( this, is2d, true,
			    uiStrings::phrInput(uiStrings::sHorizon(1)) );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_ );
    savefldgrp_->setSaveFieldName( "Save interpolated horizon" );
}


void uiChangeHorizonDlg::attachPars()
{
    if ( !parsgrp_ ) return;

    if ( inputfld_ )
	parsgrp_->attach( alignedBelow, inputfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, parsgrp_ );

    savefldgrp_->attach( alignedBelow, parsgrp_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiChangeHorizonDlg::~uiChangeHorizonDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiChangeHorizonDlg::readHorizon()
{
    if ( !inputfld_->ctxtIOObj().ioobj_ )
	return false;

    const MultiID& mid = inputfld_->ctxtIOObj().ioobj_->key();
    EM::Horizon* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) return false;

    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();

    return true;
}


bool uiChangeHorizonDlg::doProcessing()
{
    return is2d_ ? doProcessing2D() : doProcessing3D();
}

bool uiChangeHorizonDlg::doProcessing2D()
{ // TODO
    return false;
}


bool uiChangeHorizonDlg::doProcessing3D()
{
    MouseCursorChanger chgr( MouseCursor::Wait );
    bool change = false;
    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;
    mDynamicCastGet(EM::Horizon3D*,usedhor3d,usedhor)
    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
    if ( !usedhor3d || !hor3d )
	return false;

    uiTaskRunner dlg( this );
    PtrMan<Array2D<float> > arr = hor3d->createArray2D();
    if ( !arr )
    {
	uiString msg = tr("Cannot create 2D array");
	ErrMsg( msg.getFullString() );
	return false;
    }

    const StepInterval<int> rowrg = hor3d->geometry().rowRange();
    const StepInterval<int> colrg = hor3d->geometry().colRange( -1 );
    PtrMan<Executor> worker = getWorker( *arr, rowrg, colrg );
    if ( !worker )
	return false;

    if ( !TaskRunner::execute(&dlg,*worker) )
	return false;

    if ( hor3d != usedhor3d )
    {
	const BinID start( rowrg.start, colrg.start );
	const BinID step( rowrg.step, colrg.step );
	usedhor3d->geometry().geometryElement()->setArray(
					    start, step, arr, false );
    }
    else
    {
	const char* undodesc = usedhor3d==hor3d ? undoText() : nullptr;
	const bool res = usedhor3d->setArray2D( *arr,
				    fillUdfsOnly(), undodesc, false );
	change = res;
    }

    if ( change )
	EM::EMM().undo(usedhor3d->id()).setUserInteractionEnd(
		EM::EMM().undo(usedhor3d->id()).lastEventID() );

    return true;
}


bool uiChangeHorizonDlg::acceptOK( CallBacker* cb )
{
    if ( inputfld_ && !inputfld_->ioobj(true) )
    {
	uiMSG().error( uiStrings::phrSelect(mJoinUiStrs(
				sInput().toLower(),sHorizon().toLower())) );
	return false;
    }

    if ( !horizon_ && !readHorizon() )
    {
	uiMSG().error(
		uiStrings::phrCannotRead(uiStrings::sHorizon().toLower()) );
	return false;
    }

    if ( !savefldgrp_->acceptOK(cb) )
	return false;

    if ( !doProcessing() )
	return false;

    if ( saveFldGrp()->displayNewHorizon() || !saveFldGrp()->getNewHorizon() )
	horReadyForDisplay.trigger();

    const bool res = savefldgrp_->saveHorizon();
    if ( res )
	uiMSG().message( tr("Horizon sucessfully changed") );

    return false;
}


//---- uiFilterHorizonDlg

uiFilterHorizonDlg::uiFilterHorizonDlg( uiParent* p, EM::Horizon* hor )
    : uiChangeHorizonDlg(p,hor,false,uiStrings::phrJoinStrings(
		    uiStrings::sHorizon(),uiStrings::sFiltering().toLower()))
{
    Array2DFilterPars filterpars( Stats::Median );
    parsgrp_ = new uiArr2DFilterPars( this, &filterpars );
    attachPars();
}


uiFilterHorizonDlg::~uiFilterHorizonDlg()
{}


Executor* uiFilterHorizonDlg::getWorker( Array2D<float>& a2d,
					 const StepInterval<int>& rowrg,
					 const StepInterval<int>& colrg )
{
    Array2DFilterPars pars = ((uiArr2DFilterPars*)parsgrp_)->getInput();
    auto* exec = new Array2DFilterer<float>( a2d, pars );
    exec->setName( "Filtering Horizon" );
    return exec;
}
