/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocbatchsetup.cc,v 1.2 2008-08-12 18:16:07 cvskris Exp $";

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
#include "uimsg.h"


namespace VolProc
{

uiBatchSetup::uiBatchSetup( uiParent* p, const IOPar* extraomf )
    : uiFullBatchDialog( p,
	    uiFullBatchDialog::Setup("Volume Processing output")
	    .procprognm("process_volume" ) )
    , extraomf_( extraomf )
    , setupctxt_(*mGetCtxtIOObj(VolProcessing,Misc))
    , outputctxt_(*mMkCtxtIOObj(SeisTrc))
{
    setTitleText( "Create volume output" );
    setupctxt_.ctxt.forread = true;
    setupsel_ = new uiIOObjSel( uppgrp_, setupctxt_ );

    possubsel_ = new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, setupsel_ );

    outputctxt_.ctxt.forread = false;
    outputsel_ = new uiSeisSel( uppgrp_, outputctxt_,
	    			uiSeisSel::Setup(Seis::Vol) );
    outputsel_->attach( alignedBelow, possubsel_ );

    uppgrp_->setHAlignObj( setupsel_ );

    addStdFields( false, true );
}

uiBatchSetup::~uiBatchSetup()
{
    delete setupctxt_.ioobj; delete &setupctxt_;
    delete outputctxt_.ioobj; delete &outputctxt_;
}


bool uiBatchSetup::prepareProcessing()
{
    if ( !setupsel_->commitInput(true) )
    {
	uiMSG().error("Please select a setup");
	return false;
    }

    if ( !outputsel_->commitInput(true) )
    {
	uiMSG().error("Please enter an output name");
	return false;
    }

    return true;
}


bool uiBatchSetup::fillPar( IOPar& par )
{
    if ( !setupctxt_.ioobj || !outputctxt_.ioobj )
	return false; 

    par.set( VolProcessingTranslatorGroup::sKeyChainID(),
	    setupctxt_.ioobj->key() );

    possubsel_->fillPar( par );

    par.set( VolProcessingTranslatorGroup::sKeyOutputID(),
	     outputctxt_.ioobj->key() );
    if ( extraomf_ )
    {
	outputctxt_.ioobj->pars().merge( *extraomf_ );
	if ( !IOM().commitChanges( *outputctxt_.ioobj ) )
	{
	    uiMSG().error("Cannot write .omf file, check file permissions");
	    return false;
	}
    }

    return true;
}

}; //namespace


