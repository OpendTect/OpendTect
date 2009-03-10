/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uibatchtime2depthsetup.cc,v 1.1 2009-03-10 12:46:51 cvskris Exp $";

#include "uibatchtime2depthsetup.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "seistrctr.h"
#include "uipossubsel.h"
#include "uimsg.h"
#include "uiveldesc.h"

uiBatchTime2DepthSetup::uiBatchTime2DepthSetup( uiParent* p )
    : uiFullBatchDialog( p,
	uiFullBatchDialog::Setup("Time to depth conversion")
	    .procprognm("process_time2depth" ) )
    , velctxt_(*new CtxtIOObj(uiVelSel::ioContext() ) )
    , outputctxt_( *mMkCtxtIOObj(SeisTrc) )
    , inputctxt_( *mMkCtxtIOObj(SeisTrc) )
{
    setTitleText( "Time to depth conversion" );
    velctxt_.ctxt.forread = true;
    uiSeisSel::Setup velsetup( Seis::Vol );
    velsetup.seltxt( "Velocity Model" );

    velsel_ = new uiVelSel( uppgrp_, velctxt_, velsetup );

    inputctxt_.ctxt.forread = true;
    inputsel_ = new uiSeisSel( uppgrp_, inputctxt_,uiSeisSel::Setup(Seis::Vol));
    inputsel_->attach( alignedBelow, velsel_ );

    possubsel_ =  new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, inputsel_ );

    outputctxt_.ctxt.forread = false;
    outputsel_ = new uiSeisSel(uppgrp_,outputctxt_,uiSeisSel::Setup(Seis::Vol));
    outputsel_->attach( alignedBelow, possubsel_ );

    uppgrp_->setHAlignObj( possubsel_ );

    addStdFields( false, true );
}


uiBatchTime2DepthSetup::~uiBatchTime2DepthSetup()
{
    delete inputctxt_.ioobj; delete &inputctxt_;
    delete velctxt_.ioobj; delete &velctxt_;
    delete outputctxt_.ioobj; delete &outputctxt_;
}


bool uiBatchTime2DepthSetup::prepareProcessing()
{
    if ( !velsel_->commitInput(true) )
    {
	uiMSG().error("Please select a velocity volume");
	return false;
    }

    if ( !inputsel_->commitInput(true) )
    {
	uiMSG().error("Please select an input volume");
	return false;
    }

    if ( !outputsel_->commitInput(true) )
    {
	uiMSG().error("Please enter an output name");
	return false;
    }

    outputctxt_.ioobj->pars().set( sKey::ZDomain, sKey::Depth );
    if ( !IOM().commitChanges(*outputctxt_.ioobj) )
    {
	uiMSG().error("Cannot write to database",
		      "Check permissions in Seismics directory" );
	return false;
    }

    return true;
}


bool uiBatchTime2DepthSetup::fillPar( IOPar& par )
{
    if ( !inputctxt_.ioobj || !outputctxt_.ioobj || !velctxt_.ioobj )
	return false;

    par.set( "Input volume",  inputctxt_.ioobj->key() );
    possubsel_->fillPar( par );
    par.set( "Output volume", outputctxt_.ioobj->key() );
    //Set depthdomain in output's omf?

    par.set( "Velocity model", velctxt_.ioobj->key() );

    return true;
}
