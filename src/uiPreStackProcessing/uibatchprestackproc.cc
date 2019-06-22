/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uibatchprestackproc.h"

#include "ioobjctxt.h"
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

uiBatchProcSetup::uiBatchProcSetup( uiParent* p, bool is2d )
    : uiBatchProcDlg(p,uiString::empty(),false,
		     Batch::JobSpec::PreStack)
    , is2d_(is2d)
{
    setHelpKey( mODHelpKey(mPreStackBatchProcSetupHelpID) );

    uiString dlgnm = toUiString("%1 %2 %3").arg(uiStrings::sPreStack())
			       .arg(is2d_ ? uiStrings::s2D() : uiStrings::s3D())
			       .arg(uiStrings::sProcessing());
    setCaption( dlgnm );
    uiString lbl = tr("Processing Setup");
    chainsel_ = new uiProcSel( pargrp_, lbl, 0 );
    chainsel_->selectionDone.notify( mCB(this,uiBatchProcSetup,setupSelCB) );

    const Seis::GeomType gt = is2d_ ? Seis::LinePS : Seis::VolPS;
    inputsel_ = new uiSeisSel( pargrp_, uiSeisSel::ioContext(gt,true),
				uiSeisSel::Setup(gt) );
    inputsel_->attach( alignedBelow, chainsel_ );

    possubsel_ =  new uiPosSubSel( pargrp_, uiPosSubSel::Setup(is2d,false) );
    possubsel_->attach( alignedBelow, inputsel_ );

    outputsel_ = new uiSeisSel( pargrp_, uiSeisSel::ioContext(gt,false),
				uiSeisSel::Setup(gt) );
    outputsel_->attach( alignedBelow, possubsel_ );
    pargrp_->setHAlignObj( outputsel_ );
}


uiBatchProcSetup::~uiBatchProcSetup()
{
}


void uiBatchProcSetup::getJobName( BufferString& jobnm) const
{
    jobnm = outputsel_->getInput();
}


void uiBatchProcSetup::setupSelCB( CallBacker* )
{
    inputsel_->display( true );

    DBKey chainmid;
    PtrMan<IOObj> setupioobj = 0;
    if ( chainsel_->getSel(chainmid) )
	setupioobj = chainmid.getIOObj();

    if ( !setupioobj )
	return;

    mDeclareAndTryAlloc(PreStack::ProcessManager*,procman,
			PreStack::ProcessManager);
    if ( !procman )
	return;

    uiString errmsg;
    if ( !PreStackProcTranslator::retrieve(*procman,setupioobj,errmsg) )
    {
	delete procman;
	return;
    }

    if ( !procman->needsPreStackInput() )
	inputsel_->display( false );

    delete procman;
}


bool uiBatchProcSetup::prepareProcessing()
{
    DBKey chainmid;
    PtrMan<IOObj> ioobj = 0;
    if ( chainsel_->getSel(chainmid) )
	ioobj = chainmid.getIOObj();

    if ( !ioobj )
    {
	uiMSG().error( uiStrings::phrSelect(tr("a processing setup")) );
	return false;
    }

    if ( inputsel_->attachObj()->isDisplayed() && !inputsel_->commitInput() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("an input volume")) );
	return false;
    }

    if ( !outputsel_->commitInput() )
    {
	if ( outputsel_->isEmpty() )
	    uiMSG().error( uiStrings::phrEnter(tr("an output name")) );
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

    DBKey mid;
    if ( !chainsel_->getSel(mid) )
	return false;

    if ( inioobj )
	par.set( ProcessManager::sKeyInputData(), inioobj->key() );

    possubsel_->fillPar( par );
    par.set( ProcessManager::sKeyOutputData(), outioobj->key() );
    //Set depthdomain in output's omf?

    par.set( ProcessManager::sKeySetup(), mid );

    Seis::GeomType geom = is2d_ ? Seis::LinePS : Seis::VolPS;
    Seis::putInPar( geom, par );
    return true;
}
} // namespace PreStack
