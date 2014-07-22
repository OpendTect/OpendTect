/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibatchprestackproc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "seistrctr.h"
#include "uipossubsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uibatchjobdispatchersel.h"
#include "uiprestackprocessorsel.h"
#include "prestackprocessor.h"
#include "od_helpids.h"

namespace PreStack
{

uiBatchProcSetup::uiBatchProcSetup( uiParent* p, bool is2d )
    : uiDialog(p,Setup(tr("Prestack Processing"),mNoDlgTitle,
                        mODHelpKey(mPreStackBatchProcSetupHelpID)))
    , is2d_( is2d )
{
    chainsel_ = new uiProcSel( this, "Setup", 0 );

    const Seis::GeomType gt = is2d_ ? Seis::LinePS : Seis::VolPS;
    inputsel_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,true),
				uiSeisSel::Setup(gt) );
    inputsel_->attach( alignedBelow, chainsel_ );

    possubsel_ =  new uiPosSubSel( this, uiPosSubSel::Setup(is2d,false) );
    possubsel_->attach( alignedBelow, inputsel_ );

    outputsel_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,false),
				uiSeisSel::Setup(gt) );
    outputsel_->attach( alignedBelow, possubsel_ );
    outputsel_->selectionDone.notify(
				 mCB(this,uiBatchProcSetup,outputNameChangeCB));

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::PreStack );
    batchfld_->attach( alignedBelow, outputsel_ );

    outputNameChangeCB( 0 );
}


uiBatchProcSetup::~uiBatchProcSetup()
{
}


void uiBatchProcSetup::outputNameChangeCB( CallBacker* )
{
    const IOObj* ioobj = outputsel_->ioobj( true );
    if ( ioobj )
	batchfld_->setJobName( ioobj->name() );
}


bool uiBatchProcSetup::prepareProcessing()
{
    MultiID chainmid;
    PtrMan<IOObj> ioobj = 0;
    if ( chainsel_->getSel(chainmid) )
	ioobj = IOM().get( chainmid );

    if ( !ioobj )
    {
	uiMSG().error(tr("Please select a processing setup"));
	return false;
    }

    if ( !inputsel_->commitInput() )
    {
	uiMSG().error(tr("Please select an input volume"));
	return false;
    }

    if ( !outputsel_->commitInput() )
    {
	if ( outputsel_->isEmpty() )
	    uiMSG().error(tr("Please enter an output name"));
	return false;
    }

    return true;
}


bool uiBatchProcSetup::fillPar()
{
    IOPar& par = batchfld_->jobSpec().pars_;
    const IOObj* inioobj = inputsel_->ioobj( true );
    const IOObj* outioobj = outputsel_->ioobj( true );
    if ( !inioobj || !outioobj )
	return false;

    MultiID mid;
    if ( !chainsel_->getSel(mid) )
	return false;

    par.set( ProcessManager::sKeyInputData(), inioobj->key() );
    possubsel_->fillPar( par );
    par.set( ProcessManager::sKeyOutputData(), outioobj->key() );
    //Set depthdomain in output's omf?

    par.set( ProcessManager::sKeySetup(), mid );

    Seis::GeomType geom = is2d_ ? Seis::LinePS : Seis::VolPS;
    Seis::putInPar( geom, par );
    return true;
}


bool uiBatchProcSetup::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( outputsel_->ioobj()->name() );
    return batchfld_->start();
}


} // namespace PreStack
