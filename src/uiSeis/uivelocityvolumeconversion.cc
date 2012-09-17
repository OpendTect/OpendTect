/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityvolumeconversion.cc,v 1.6 2011/04/14 12:14:30 cvshelene Exp $";

#include "uivelocityvolumeconversion.h"

#include "ctxtioobj.h"
#include "seisbounds.h"
#include "seistrctr.h"
#include "seisread.h"
#include "survinfo.h"
#include "velocityvolumeconversion.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiveldesc.h"
#include "uicombobox.h"

Vel::uiBatchVolumeConversion::uiBatchVolumeConversion( uiParent* p )
    : uiFullBatchDialog( p,
	uiFullBatchDialog::Setup("Velocity conversion")
	    .procprognm("od_process_velocityconv" ) )
{
    setTitleText( "Velocity conversion" );
    setHelpID( "103.2.14" );

    IOObjContext velctxt = uiVelSel::ioContext();
    velctxt.forread = true;
    uiSeisSel::Setup velsetup( Seis::Vol );
    velsetup.seltxt( "Input velocity model" );
    input_ = new uiVelSel( uppgrp_, velctxt, velsetup );
    input_->selectionDone.notify(
	    mCB(this,Vel::uiBatchVolumeConversion,inputChangeCB ));

    possubsel_ =  new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,false) );
    possubsel_->attach( alignedBelow, input_ );

    outputveltype_ = new uiLabeledComboBox( uppgrp_, "Output velocity type" );
    outputveltype_->attach( alignedBelow, possubsel_ );

    IOObjContext outputctxt = SeisTrcTranslatorGroup::ioContext();
    outputctxt.forread = false;
    outputsel_ = new uiSeisSel(uppgrp_,outputctxt,uiSeisSel::Setup(Seis::Vol));
    outputsel_->attach( alignedBelow, outputveltype_ );

    uppgrp_->setHAlignObj( possubsel_ );

    setParFileNmDef( "velocity_conversion" );

    addStdFields( false, true );

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
	outputveltype_->box()->textOfItem(outputveltype_->box()->currentItem());

    TypeSet<VelocityDesc::Type> types;
    if ( SI().zIsTime() ) types += VelocityDesc::RMS;
    types += VelocityDesc::Interval;
    types += VelocityDesc::Avg;
    types -= desc.type_;

    outputveltype_->box()->setEmpty();

    for ( int idx=0; idx<types.size(); idx++ )
    {
	outputveltype_->box()->addItem(
		VelocityDesc::getTypeString(types[idx] ) );
    }

    outputveltype_->box()->setCurrentItem( oldoutputtype );

    SeisTrcReader reader( velioobj );
    if ( !reader.prepareWork( Seis::PreScan ) )
	return;

    PtrMan<Seis::Bounds> bounds = reader.getBounds();
    if ( !bounds )
	return;

    mDynamicCastGet( const Seis::Bounds3D*, bounds3d, bounds.ptr() );
    if ( !bounds3d )
	return;

    possubsel_->setInputLimit( bounds3d->cs_ );
}


bool Vel::uiBatchVolumeConversion::fillPar( IOPar& par )
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
	uiMSG().error("Could not read velocity information on input" );
	return false;
    }

    if ( inputveldesc.type_!=VelocityDesc::Interval &&
         inputveldesc.type_!=VelocityDesc::Avg &&
	 inputveldesc.type_!=VelocityDesc::RMS )
    {
	uiMSG().error("Only RMS, Avg or Interval velcities can be converted");
	return false;
    }

    const int outputvelidx = outputveltype_->box()->currentItem();
    if ( outputvelidx==-1 )
    {
	pErrMsg("The velocity is not set");
	return false;
    }

    const FixedString outputtype =
	outputveltype_->box()->textOfItem( outputvelidx );

    VelocityDesc outputdesc;
    if ( !VelocityDesc::parseEnumType( outputtype, outputdesc.type_ ) )
    {
	pErrMsg("Imparsable velocity type");
	return false;
    }

    outputdesc.fillPar( par );

    par.set( Vel::VolumeConverter::sKeyInput(),  velioobj->key() );

    possubsel_->fillPar( par );
    par.set( Vel::VolumeConverter::sKeyOutput(), outputioobj->key() );

    return true;
}
