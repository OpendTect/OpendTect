/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocbatchsetup.cc,v 1.5 2009-03-24 12:33:52 cvsbert Exp $";

#include "uivolprocbatchsetup.h"
#include "volproctrans.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "seistrctr.h"
#include "ioman.h"
#include "ioobj.h"

#include "uipossubsel.h"
#include "uiioobjsel.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uiveldesc.h"
#include "uimsg.h"



VolProc::uiBatchSetup::uiBatchSetup( uiParent* p, const IOPar* extraomf,
			    const IOObj* initialioobj )
    : uiFullBatchDialog( p,
	    uiFullBatchDialog::Setup("Volume Processing output")
	    .procprognm("process_volume" ) )
    , extraomf_( extraomf )
    , setupctxt_(*mMkCtxtIOObj(VolProcessing))
    , outputctxt_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
{
    setCtrlStyle( DoAndStay );
    if ( initialioobj )
	setupctxt_.setObj( initialioobj->clone() );
    setTitleText( "Build output volume" );
    setupctxt_.ctxt.forread = true;
    setupsel_ = new uiIOObjSel( uppgrp_, setupctxt_ );

    possubsel_ = new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, setupsel_ );

    outputsel_ = new uiSeisSel( uppgrp_, outputctxt_,
	    			uiSeisSel::Setup(Seis::Vol) );
    outputsel_->attach( alignedBelow, possubsel_ );
    outputsel_->selectiondone.notify( mCB(this,uiBatchSetup,outSel) );

    static const char* typnms[] = { "Velocity", "Density", "Impedance", 0 };
    outisvelfld_ = new uiGenInput( uppgrp_, "Output type",
				     BoolInpSpec(true,"Velocity","Other") );
    outisvelfld_->attach( alignedBelow, outputsel_ );
    outisvelfld_->valuechanged.notify( mCB(this,uiBatchSetup,outTypChg) );
    uiveldesc_ = new uiVelocityDesc( uppgrp_ );
    uiveldesc_->attach( alignedBelow, outisvelfld_ );

    uppgrp_->setHAlignObj( setupsel_ );

    addStdFields( false, true );
}

VolProc::uiBatchSetup::~uiBatchSetup()
{
    delete setupctxt_.ioobj; delete &setupctxt_;
    delete outputctxt_.ioobj; delete &outputctxt_;
}


bool VolProc::uiBatchSetup::prepareProcessing()
{
    if ( !setupsel_->commitInput() )
    {
	uiMSG().error("Please select a setup");
	return false;
    }

    if ( !outputsel_->commitInput() )
    {
	uiMSG().error("Please enter an output name");
	return false;
    }

    return true;
}


bool VolProc::uiBatchSetup::fillPar( IOPar& par )
{
    if ( !setupctxt_.ioobj || !outputctxt_.ioobj )
	return false; 

    par.set( VolProcessingTranslatorGroup::sKeyChainID(),
	    setupctxt_.ioobj->key() );

    possubsel_->fillPar( par );

    par.set( VolProcessingTranslatorGroup::sKeyOutputID(),
	     outputctxt_.ioobj->key() );

    bool needcommit = false; bool commitfailed = false;
    if ( extraomf_ )
    {
	outputctxt_.ioobj->pars().merge( *extraomf_ );
	needcommit = true;
    }
    if ( outisvelfld_->getBoolValue() )
    {
	commitfailed = !uiveldesc_->updateAndCommit( *outputctxt_.ioobj );
	needcommit = false;
    }

    if ( commitfailed || needcommit )
    {
	if ( commitfailed || !IOM().commitChanges( *outputctxt_.ioobj ) )
	{
	    uiMSG().error("Cannot write .omf file, check file permissions");
	    return false;
	}
    }

    return true;
}


void VolProc::uiBatchSetup::outTypChg( CallBacker* )
{
    uiveldesc_->display( outisvelfld_->getBoolValue() );
}


void VolProc::uiBatchSetup::outSel( CallBacker* )
{
    if ( !outputsel_->commitInput() || !outputctxt_.ioobj ) return;
    VelocityDesc vd;
    const bool isvel = vd.usePar( outputctxt_.ioobj->pars() );
    uiveldesc_->set( vd );
    outisvelfld_->setValue( isvel );
}
