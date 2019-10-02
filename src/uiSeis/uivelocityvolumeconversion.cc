/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uivelocityvolumeconversion.h"

#include "fullsubsel.h"
#include "ioobjctxt.h"
#include "seistrctr.h"
#include "seisprovider.h"
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
    IOObjContext velctxt = uiVelSel::ioContext();
    velctxt.forread_ = true;
    uiSeisSel::Setup velsetup( Seis::Vol );
    velsetup.seltxt( tr("Input velocity model") );
    input_ = new uiVelSel( this, velctxt, velsetup );
    input_->selectionDone.notify(
	    mCB(this,Vel::uiBatchVolumeConversion,inputChangeCB ) );

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

    inputChangeCB( 0 );
}


void Vel::uiBatchVolumeConversion::inputChangeCB( CallBacker* )
{
    const IOObj* velioobj = input_->ioobj( true );
    if ( !velioobj )
	return;

    VelocityDesc desc;
    if ( !desc.usePar( velioobj->pars() ) ||
	    (desc.type_!=VelocityDesc::Interval &&
	     desc.type_!=VelocityDesc::RMS &&
	     desc.type_!=VelocityDesc::Avg ) )
	return;

    FixedString oldoutputtype =
	outputveltype_->box()->itemText(outputveltype_->box()->currentItem());

    TypeSet<VelocityDesc::Type> types;
    if ( SI().zIsTime() ) types += VelocityDesc::RMS;
    types += VelocityDesc::Interval;
    types += VelocityDesc::Avg;
    types -= desc.type_;

    outputveltype_->box()->setEmpty();

    for ( int idx=0; idx<types.size(); idx++ )
    {
	outputveltype_->box()->addItem(VelocityDesc::toUiString(types[idx]));
    }

    outputveltype_->box()->setCurrentItem( oldoutputtype );

    uiRetVal uirv;
    PtrMan<Seis::Provider> provider = Seis::Provider::create(
					velioobj->key(), &uirv );
    if ( !provider )
	{ uiMSG().error( uirv ); return; }

    const auto* prov3d = provider->as3D();
    if ( prov3d )
    {
	Survey::FullSubSel fss;
	prov3d->getFullSubSel( fss );
	possubsel_->setInputLimit( TrcKeyZSampling(fss) );
    }
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
    if ( !inputveldesc.usePar( velioobj->pars() ) )
    {
	uiMSG().error(uiStrings::phrCannotRead(
					tr("velocity information on input")) );
	return false;
    }

    if ( inputveldesc.type_!=VelocityDesc::Interval &&
         inputveldesc.type_!=VelocityDesc::Avg &&
	 inputveldesc.type_!=VelocityDesc::RMS )
    {
	uiMSG().error(tr("Only RMS, Avg or Interval"
			 " velcities can be converted"));
	return false;
    }

    const int outputvelidx = outputveltype_->box()->currentItem();
    if ( outputvelidx==-1 )
    {
	pErrMsg("The velocity is not set");
	return false;
    }

    const FixedString outputtype =
	outputveltype_->box()->itemText( outputvelidx );

    VelocityDesc outputdesc;
    if ( !VelocityDesc::TypeDef().parse( outputtype, outputdesc.type_ ) )
    {
	pErrMsg("Imparsable velocity type");
	return false;
    }
    else
	outputdesc.fillPar( outputioobj->pars() );

    outputioobj->commitChanges();

    IOPar& par = batchfld_->jobSpec().pars_;
    outputdesc.fillPar( par );

    par.set( Vel::VolumeConverter::sKeyInput(),  velioobj->key() );

    possubsel_->fillPar( par );
    par.set( Vel::VolumeConverter::sKeyOutput(), outputioobj->key() );

    return true;
}


bool Vel::uiBatchVolumeConversion::acceptOK()
{
    if ( !fillPar() )
	return false;

    batchfld_->setJobName( outputsel_->ioobj()->name() );
    return batchfld_->start();
}
