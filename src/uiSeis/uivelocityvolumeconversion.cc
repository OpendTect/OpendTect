/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocityvolumeconversion.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seisbounds.h"
#include "seistrctr.h"
#include "seisread.h"
#include "survinfo.h"
#include "velocityvolumeconversion.h"

#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiveldesc.h"
#include "od_helpids.h"

Vel::uiBatchVolumeConversion::uiBatchVolumeConversion( uiParent* p )
    : uiDialog( p, uiDialog::Setup(tr("Velocity conversion"),
			tr("Velocity conversion"),
			mODHelpKey(mVelBatchVolumeConversionHelpID) ) )
{
    input_ = new uiVelSel( this, tr("Input velocity model") );
    mAttachCB( input_->selectionDone, uiBatchVolumeConversion::inputChangeCB );

    possubsel_ =  new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    possubsel_->attach( alignedBelow, input_ );

    outputveltype_ = new uiLabeledComboBox( this, tr("Output velocity type") );
    outputveltype_->attach( alignedBelow, possubsel_ );

    IOObjContext outputctxt = SeisTrcTranslatorGroup::ioContext();
    outputctxt.forread_ = false;
    outputsel_ = new uiSeisSel(this, outputctxt,uiSeisSel::Setup(Seis::Vol));
    outputsel_->attach( alignedBelow, outputveltype_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::VelConv );
    batchfld_->attach( alignedBelow, outputsel_ );

    mAttachCB( postFinalize(), uiBatchVolumeConversion::inputChangeCB );
}


Vel::uiBatchVolumeConversion::~uiBatchVolumeConversion()
{
    detachAllNotifiers();
}


void Vel::uiBatchVolumeConversion::initDlgCB( CallBacker* )
{
    inputChangeCB( nullptr );
}


void Vel::uiBatchVolumeConversion::inputChangeCB( CallBacker* )
{
    const IOObj* velioobj = input_->ioobj( true );
    if ( !velioobj )
	return;

    VelocityDesc desc;
    if ( !desc.usePar(velioobj->pars()) || !desc.isVelocity() )
	return;

    StringView oldoutputtype =
	outputveltype_->box()->textOfItem(outputveltype_->box()->currentItem());

    TypeSet<Type> types;
    if ( SI().zIsTime() )
	types += RMS;

    types += Interval;
    types += Avg;
    types -= desc.type_;

    outputveltype_->box()->setEmpty();

    for ( const auto& veltype : types )
	outputveltype_->box()->addItem( TypeDef().toUiString(veltype) );

    outputveltype_->box()->setCurrentItem( oldoutputtype );

    SeisTrcReader reader( *velioobj );
    if ( !reader.prepareWork(Seis::PreScan) )
	return;

    PtrMan<Seis::Bounds> bounds = reader.getBounds();
    if ( !bounds )
	return;

    mDynamicCastGet( const Seis::Bounds3D*, bounds3d, bounds.ptr() );
    if ( !bounds3d )
	return;

    possubsel_->setInputLimit( bounds3d->tkzs_ );
}


bool Vel::uiBatchVolumeConversion::fillPar()
{
    const IOObj* outputioobj = outputsel_->ioobj(false);
    if ( !outputioobj )
	return false;

    const IOObj* velioobj = input_->ioobj( false );
    if ( !velioobj )
	return false;

    VelocityDesc inputveldesc;
    if ( !inputveldesc.usePar(velioobj->pars()) )
    {
	uiMSG().error(tr("Could not read velocity information on input") );
	return false;
    }

    if ( !inputveldesc.isVelocity() )
    {
	uiMSG().error(tr("Only RMS, Avg or Interval"
			 " velocities can be converted"));
	return false;
    }

    const int outputvelidx = outputveltype_->box()->currentItem();
    if ( outputvelidx==-1 )
    {
	pErrMsg("The velocity is not set");
	return false;
    }

    const StringView outputtype =
			outputveltype_->box()->textOfItem( outputvelidx );
    VelocityDesc outputdesc;
    if ( TypeDef().parse(outputtype,outputdesc.type_) )
    {
	outputdesc.setUnit( inputveldesc.getUnit() );
	outputdesc.fillPar( outputioobj->pars() );
    }
    else
    {
	pErrMsg("Imparsable velocity type");
	return false;
    }

    if ( SeisStoreAccess::zDomain(velioobj).fillPar(outputioobj->pars()) &&
	 !IOM().commitChanges( *outputioobj ) )
    {
	uiMSG().error(uiStrings::phrCannotWriteDBEntry(outputioobj->uiName()));
	return false;
    }

    IOPar& par = batchfld_->jobSpec().pars_;
    outputdesc.fillPar( par );

    par.set( VolumeConverter::sKeyInput(),  velioobj->key() );

    possubsel_->fillPar( par );
    par.set( VolumeConverter::sKeyOutput(), outputioobj->key() );

    batchfld_->saveProcPars( *outputioobj );
    return true;
}


bool Vel::uiBatchVolumeConversion::acceptOK( CallBacker* )
{
    if ( !fillPar() )
	return false;

    batchfld_->setJobName( outputsel_->ioobj()->name() );
    return batchfld_->start();
}
