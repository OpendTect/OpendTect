/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocbatchsetup.cc,v 1.3 2009-03-19 13:27:12 cvsbert Exp $";

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
#include "uimsg.h"


namespace VolProc
{

uiBatchSetup::uiBatchSetup( uiParent* p, const IOPar* extraomf,
			    const IOObj* initialioobj )
    : uiFullBatchDialog( p,
	    uiFullBatchDialog::Setup("Volume Processing output")
	    .procprognm("process_volume" ) )
    , extraomf_( extraomf )
    , setupctxt_(*mGetCtxtIOObj(VolProcessing,Misc))
    , outputctxt_(*mMkCtxtIOObj(SeisTrc))
{
    setCtrlStyle( DoAndStay );
    if ( initialioobj )
	setupctxt_.setObj( initialioobj->clone() );
    setTitleText( "Build output volume" );
    setupctxt_.ctxt.forread = true;
    setupsel_ = new uiIOObjSel( uppgrp_, setupctxt_ );

    possubsel_ = new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, setupsel_ );

    outputctxt_.ctxt.forread = false;
    outputsel_ = new uiSeisSel( uppgrp_, outputctxt_,
	    			uiSeisSel::Setup(Seis::Vol) );
    outputsel_->attach( alignedBelow, possubsel_ );

    static const char* typnms[] = { "Velocity", "Density", "Impedance", 0 };
    outputtypefld_ = new uiGenInput( uppgrp_, "Output cube type",
				     StringListInpSpec(typnms) );
    outputtypefld_->attach( alignedBelow, outputsel_ );

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

    //TODO : use outputtypefld_ to set type

    return true;
}

}; //namespace


