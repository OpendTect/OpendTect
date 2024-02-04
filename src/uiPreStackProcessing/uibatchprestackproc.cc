/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchprestackproc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "prestackprocessor.h"
#include "prestackprocessortransl.h"
#include "seistrctr.h"
#include "uibatchjobdispatchersel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiprestackprocessorsel.h"
#include "uiseissel.h"

#include "od_helpids.h"

namespace PreStack
{

uiBatchProcSetup::uiBatchProcSetup( uiParent* p, OD::GeomSystem gs, int openidx,
				    const uiStringSet* usemethods )
    : uiBatchProcDlg(p,uiString::empty(),false,Batch::JobSpec::PreStack)
    , gs_(gs)
{
    setHelpKey( mODHelpKey(mPreStackBatchProcSetupHelpID) );

    const bool is3d = ::is3D( gs_ );
    const bool is2d = ::is2D( gs_ );
    const uiString dlgnm = toUiString("%1 %2 %3").arg(uiStrings::sPreStack())
			       .arg(is2d ? uiStrings::s2D()
					 : (is3d ? uiStrings::s3D()
						 : tr("Synthetic")))
			       .arg(uiStrings::sProcessing());
    setCaption( dlgnm );
    chainsel_ = new uiProcSel( pargrp_, uiStrings::sSetup(), gs_, openidx,
			       usemethods );

    const Seis::GeomType gt = ::is3D(gs_) ? Seis::VolPS : Seis::LinePS;
    inputsel_ = new uiSeisSel( pargrp_, uiSeisSel::ioContext(gt,true),
			       uiSeisSel::Setup(gt) );
    inputsel_->attach( alignedBelow, chainsel_ );

    possubsel_ =  new uiPosSubSel( pargrp_, uiPosSubSel::Setup(!is3d,false) );
    possubsel_->attach( alignedBelow, inputsel_ );

    outputsel_ = new uiSeisSel( pargrp_, uiSeisSel::ioContext(gt,false),
				uiSeisSel::Setup(gt) );
    outputsel_->attach( alignedBelow, possubsel_ );
    pargrp_->setHAlignObj( outputsel_ );

    mAttachCB( postFinalize(), uiBatchProcSetup::initDlgCB );
}


uiBatchProcSetup::~uiBatchProcSetup()
{
    detachAllNotifiers();
}


bool uiBatchProcSetup::isOK() const
{
    const bool noerr = true;
    MultiID chainid;
    if ( !chainsel_->getSel(chainid,noerr) || chainid.isUdf() )
	return false;

    if ( inputsel_->attachObj()->isDisplayed() && !inputsel_->ioobj(noerr) )
	return false;

    if ( !outputsel_->ioobj(noerr) )
	return false;

    return true;
}


void uiBatchProcSetup::getJobName( BufferString& jobnm) const
{
    jobnm = outputsel_->getInput();
}


void uiBatchProcSetup::initDlgCB( CallBacker* )
{
    mAttachCB( chainsel_->selectionDone, uiBatchProcSetup::setupSelCB );
}


void uiBatchProcSetup::setupSelCB( CallBacker* )
{
    inputsel_->display( true );

    MultiID chainmid;
    PtrMan<IOObj> setupioobj;
    if ( chainsel_->getSel(chainmid) && !chainmid.isUdf() )
	setupioobj = IOM().get( chainmid );

    if ( !setupioobj )
	return;

    ProcessManager procman( gs_ );
    uiString errmsg;
    if ( !PreStackProcTranslator::retrieve(procman,setupioobj,errmsg) )
	return;

    inputsel_->display( procman.needsPreStackInput() );
}


bool uiBatchProcSetup::prepareProcessing()
{
    MultiID chainmid;
    PtrMan<IOObj> ioobj;
    if ( chainsel_->getSel(chainmid) )
	ioobj = IOM().get( chainmid );

    if ( inputsel_->attachObj()->isDisplayed() && !inputsel_->commitInput() )
    {
	uiMSG().error( tr("Please select an input volume") );
	return false;
    }

    if ( !outputsel_->commitInput() )
    {
	if ( outputsel_->isEmpty() )
	    uiMSG().error( tr("Please enter an output name") );
	return false;
    }

    return true;
}


bool uiBatchProcSetup::fillPar( IOPar& par )
{
    const IOObj* inioobj = inputsel_->ioobj( true );
    const IOObj* outioobj = outputsel_->ioobj( true );
    if ( (inputsel_->attachObj()->isDisplayed() && !inioobj) || !outioobj )
	return false;

    MultiID mid;
    if ( !chainsel_->getSel(mid) )
	return false;

    if ( inioobj )
	par.set( ProcessManager::sKeyInputData(), inioobj->key() );

    possubsel_->fillPar( par );
    par.set( ProcessManager::sKeyOutputData(), outioobj->key() );
    //Set depthdomain in output's omf?

    par.set( ProcessManager::sKeySetup(), mid );

    const Seis::GeomType geom = ::is3D(gs_) ? Seis::VolPS : Seis::LinePS;
    Seis::putInPar( geom, par );
    return true;
}

} // namespace PreStack
