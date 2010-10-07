/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocityvolumeconversion.cc,v 1.3 2010-10-07 06:11:25 cvsnanne Exp $";

#include "uivelocityvolumeconversion.h"

#include "ctxtioobj.h"
#include "seisbounds.h"
#include "seistrctr.h"
#include "seisread.h"
#include "velocityvolumeconversion.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiveldesc.h"

Vel::uiBatchVolumeConversion::uiBatchVolumeConversion( uiParent* p )
    : uiFullBatchDialog( p,
	uiFullBatchDialog::Setup("Velocity conversion")
	    .procprognm("od_process_velocityconv" ) )
{
    setTitleText( "VelocityConversion conversion" );
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

    IOObjContext outputctxt = SeisTrcTranslatorGroup::ioContext();
    outputctxt.forread = false;
    outputsel_ = new uiSeisSel(uppgrp_,outputctxt,uiSeisSel::Setup(Seis::Vol));
    outputsel_->attach( alignedBelow, possubsel_ );

    uppgrp_->setHAlignObj( possubsel_ );

    setParFileNmDef( "velocity_conversion" );


    addStdFields( false, true );
}


void Vel::uiBatchVolumeConversion::inputChangeCB( CallBacker* )
{
    const IOObj* velioobj = input_->ioobj( true );
    if ( !velioobj )
	return;

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
	 inputveldesc.type_!=VelocityDesc::RMS )
    {
	uiMSG().error("Only RMS or Interval velcities can be converted");
	return false;
    }

    VelocityDesc outputdesc;
    outputdesc.type_ = inputveldesc.type_==VelocityDesc::Interval
	? VelocityDesc::RMS
	: VelocityDesc::Interval;

    outputdesc.fillPar( par );

    par.set( Vel::VolumeConverter::sKeyInput(),  velioobj->key() );
    possubsel_->fillPar( par );
    par.set( Vel::VolumeConverter::sKeyOutput(), outputioobj->key() );

    return true;
}
