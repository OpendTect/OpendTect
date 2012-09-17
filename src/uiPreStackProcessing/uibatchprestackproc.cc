/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uibatchprestackproc.cc,v 1.8 2011/10/25 09:19:26 cvskris Exp $";

#include "uibatchprestackproc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "seistrctr.h"
#include "uipossubsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiprestackprocessorsel.h"
#include "prestackprocessor.h"

namespace PreStack
{

uiBatchProcSetup::uiBatchProcSetup( uiParent* p, bool is2d )
    : uiFullBatchDialog( p,
	uiFullBatchDialog::Setup("Pre stack processing")
	.procprognm("od_process_prestack" ) )
    , outputctxt_( *uiSeisSel::mkCtxtIOObj( is2d ? Seis::LinePS : Seis::VolPS,
					    false ) )
    , inputctxt_( *uiSeisSel::mkCtxtIOObj( is2d ? Seis::LinePS : Seis::VolPS,
					    true ) )
    , is2d_( is2d )
{
    setTitleText( "Prestack processing" );
    setHelpID( "103.2.10" );

    chainsel_ = new uiProcSel( uppgrp_, "Setup", 0 );

    inputctxt_.ctxt.forread = true;
    inputsel_ = new uiSeisSel( uppgrp_,
	       inputctxt_,uiSeisSel::Setup( is2d ? Seis::LinePS : Seis::VolPS));
    inputsel_->attach( alignedBelow, chainsel_ );

    possubsel_ =  new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(is2d,false) );
    possubsel_->attach( alignedBelow, inputsel_ );

    outputctxt_.ctxt.forread = false;
    outputsel_ = new uiSeisSel( uppgrp_, outputctxt_,
		       uiSeisSel::Setup( is2d ? Seis::LinePS : Seis::VolPS ));
    outputsel_->attach( alignedBelow, possubsel_ );
    outputsel_->selectionDone.notify( mCB(this,uiBatchProcSetup,outputNameChangeCB));

    uppgrp_->setHAlignObj( possubsel_ );

    addStdFields( false, true );
    outputNameChangeCB( 0 );
}


uiBatchProcSetup::~uiBatchProcSetup()
{
    delete inputctxt_.ioobj; delete &inputctxt_;
    delete outputctxt_.ioobj; delete &outputctxt_;
}


void uiBatchProcSetup::outputNameChangeCB( CallBacker* )
{
    BufferString parfilename = "od_process_prestack";
    if ( outputsel_->ioobj(true) )
    {
	parfilename += "_";
	parfilename += outputsel_->ioobj(true)->name();
	cleanupString( parfilename.buf(), false, false, false );
    }

    setParFileNmDef( parfilename );
}


bool uiBatchProcSetup::prepareProcessing()
{
    MultiID chainmid;
    PtrMan<IOObj> ioobj = 0;
    if ( chainsel_->getSel(chainmid) )
	ioobj = IOM().get( chainmid );

    if ( !ioobj )
    {
	uiMSG().error("Please select a processing setup");
	return false;
    }

    if ( !inputsel_->commitInput() )
    {
	uiMSG().error("Please select an input volume");
	return false;
    }

    if ( !outputsel_->commitInput() )
    {
	if ( outputsel_->isEmpty() )
	    uiMSG().error("Please enter an output name");
	return false;
    }

    return true;
}


bool uiBatchProcSetup::fillPar( IOPar& par )
{
    if ( !inputctxt_.ioobj || !outputctxt_.ioobj )
	return false;

    MultiID mid;
    if ( !chainsel_->getSel(mid) )
	return false;

    par.set( ProcessManager::sKeyInputData(),  inputctxt_.ioobj->key() );
    possubsel_->fillPar( par );
    par.set( ProcessManager::sKeyOutputData(), outputctxt_.ioobj->key() );
    //Set depthdomain in output's omf?

    par.set( ProcessManager::sKeySetup(), mid );

    Seis::GeomType geom = is2d_ ? Seis::LinePS : Seis::VolPS;
    Seis::putInPar( geom, par );
    return true;
}

};
