/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uibatchtime2depthsetup.cc,v 1.9 2010-03-18 18:16:21 cvskris Exp $";

#include "uibatchtime2depthsetup.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "process_time2depth.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiveldesc.h"

uiBatchTime2DepthSetup::uiBatchTime2DepthSetup( uiParent* p )
    : uiFullBatchDialog( p,
	uiFullBatchDialog::Setup("Time to depth volume conversion")
	    .procprognm("process_time2depth" ) )
{
    setTitleText( "Time/Depth conversion" );
    setHelpID( "103.2.12" );

    directionsel_ = new uiGenInput( uppgrp_, "Direction",
	    BoolInpSpec(true, "Time to Depth", "Depth to Time", true ) );
    directionsel_->valuechanged.notify(
	    mCB(this,uiBatchTime2DepthSetup,dirChangeCB));

    IOObjContext velctxt = uiVelSel::ioContext();
    velctxt.forread = true;
    uiSeisSel::Setup velsetup( Seis::Vol );
    velsetup.seltxt( "Velocity Model" );
    velsel_ = new uiVelSel( uppgrp_, velctxt, velsetup );
    velsel_->attach( alignedBelow, directionsel_ );

    IOObjContext inputtimectxt = SeisTrcTranslatorGroup::ioContext();
    inputtimectxt.parconstraints.add( ZDomain::sKey(), ZDomain::sKeyTWT() );
    inputtimectxt.allowcnstrsabsent = SI().zIsTime();
    inputtimectxt.forread = true;
    uiSeisSel::Setup setup(Seis::Vol); setup.seltxt("Input Time Volume");
    inputtimesel_ = new uiSeisSel( uppgrp_, inputtimectxt, setup );
    inputtimesel_->attach( alignedBelow, velsel_ );

    IOObjContext inputdepthctxt = SeisTrcTranslatorGroup::ioContext();
    inputdepthctxt.parconstraints.add( ZDomain::sKey(), ZDomain::sKeyDepth() );
    inputdepthctxt.allowcnstrsabsent = !SI().zIsTime();
    inputdepthctxt.forread = true;
    setup.seltxt("Input Depth Volume");
    inputdepthsel_ = new uiSeisSel( uppgrp_, inputdepthctxt, setup );
    inputdepthsel_->attach( alignedBelow, velsel_ );

    possubsel_ =  new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,false) );
    possubsel_->attach( alignedBelow, inputtimesel_ );

    IOObjContext outputctxt = SeisTrcTranslatorGroup::ioContext();
    outputctxt.forread = false;
    outputsel_ = new uiSeisSel(uppgrp_,outputctxt,uiSeisSel::Setup(Seis::Vol));
    outputsel_->attach( alignedBelow, possubsel_ );

    uppgrp_->setHAlignObj( possubsel_ );

    addStdFields( false, true );
    dirChangeCB( 0 );
}


void uiBatchTime2DepthSetup::dirChangeCB( CallBacker* )
{
    const bool istime2depth = directionsel_->getBoolValue();
    inputtimesel_->display( istime2depth );
    inputdepthsel_->display( !istime2depth );
    outputsel_->setLabelText( istime2depth
	    ? "Ouput Depth Volume"
	    : "Output Time Volume");
}


bool uiBatchTime2DepthSetup::prepareProcessing()
{
    const IOObj* outputioobj = outputsel_->ioobj(false);
    if ( !outputioobj || !velsel_->ioobj( false ) )
	return false;

    PtrMan<IOObj> outputioobjclone = outputioobj->clone();

    if ( directionsel_->getBoolValue() )
    {
	if ( !inputtimesel_->ioobj( false ) )
	    return false;

	outputioobjclone->pars().set( ZDomain::sKey(), ZDomain::sKeyDepth() );
    }
    else
    {
	if ( !inputdepthsel_->ioobj( false ) )
	    return false;

	outputioobjclone->pars().set( ZDomain::sKey(), ZDomain::sKeyTWT() );
    }

    if ( !IOM().commitChanges(*outputioobjclone) )
    {
	uiMSG().error("Cannot write to database",
		      "Check permissions in Seismics directory" );
	return false;
    }

    return true;
}


bool uiBatchTime2DepthSetup::fillPar( IOPar& par )
{
    if ( !outputsel_->ioobj(true) || !velsel_->ioobj( true ) )
	return false;

    const IOObj* input = directionsel_->getBoolValue()
	? inputtimesel_->ioobj( true )
	: inputdepthsel_->ioobj( true );

    if ( !input )
	return false;

    par.set( ProcessTime2Depth::sKeyInputVolume(),  input->key() );
    possubsel_->fillPar( par );
    par.set( ProcessTime2Depth::sKeyOutputVolume(),
	     outputsel_->ioobj(true)->key() );
    par.set( ProcessTime2Depth::sKeyVelocityModel(),
	     velsel_->ioobj(true)->key() );
    par.setYN( ProcessTime2Depth::sKeyIsTimeToDepth(),
	       directionsel_->getBoolValue() );

    return true;
}
