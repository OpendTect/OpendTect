/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uivolprocbatchsetup.h"
#include "volproctrans.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "volprocchain.h"

#include "uibutton.h"
#include "uipossubsel.h"
#include "uiioobjsel.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uiveldesc.h"
#include "uivolprocchain.h"
#include "uimsg.h"
#include "pixmap.h"



VolProc::uiBatchSetup::uiBatchSetup( uiParent* p, const IOObj* initialsetup )
    : uiFullBatchDialog( p,
	    uiFullBatchDialog::Setup("Volume Builder: Create output")
	    .procprognm("od_process_volume" ) )
    , chain_( 0 )
{
    setTitleText( 0 );
    setHelpID( "103.2.11" );

    IOObjContext setupcontext = VolProcessingTranslatorGroup::ioContext();
    setupcontext.forread = true;
    setupsel_ = new uiIOObjSel( uppgrp_, setupcontext,
	   			"Volume Builder setup" );
    if ( initialsetup )
	setupsel_->setInput( *initialsetup );
    setupsel_->selectionDone.notify( mCB(this,uiBatchSetup,setupSelCB) );

    editsetup_ = new uiPushButton( uppgrp_, "Create",
	    ioPixmap(VolProc::uiChain::pixmapFileName()),
	    mCB(this, uiBatchSetup, editPushCB), false );
    editsetup_->attach( rightOf, setupsel_ );

    possubsel_ = new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,true) );
    possubsel_->attach( alignedBelow, setupsel_ );

    outputsel_ = new uiSeisSel( uppgrp_, uiSeisSel::ioContext(Seis::Vol,false), 
	    			uiSeisSel::Setup(Seis::Vol) );
    outputsel_->attach( alignedBelow, possubsel_ );

    uppgrp_->setHAlignObj( setupsel_ );

    setParFileNmDef( "volume_builder" );

    addStdFields( false, true );
    setupSelCB( 0 );
}


VolProc::uiBatchSetup::~uiBatchSetup()
{
    if ( chain_ ) chain_->unRef();
}


void VolProc::uiBatchSetup::editPushCB( CallBacker* )
{
    const IOObj* setupioobj = setupsel_->ioobj(true);

    if ( !chain_ )
    {
	chain_ = new VolProc::Chain;
	chain_->ref();
    }

    if ( setupioobj && chain_->storageID()!=setupioobj->key() )
    {
	BufferString errmsg;
	MouseCursorChanger mcc( MouseCursor::Wait );
	VolProcessingTranslator::retrieve( *chain_, setupioobj, errmsg );
    }

    VolProc::uiChain dlg( this, *chain_, false );

    if ( dlg.go() )
    {
	setupsel_->setInput( dlg.storageID() );
    }
} 


bool VolProc::uiBatchSetup::prepareProcessing()
{
    if ( !setupsel_->ioobj() || !outputsel_->ioobj() )
	return false;

    return true;
}


bool VolProc::uiBatchSetup::fillPar( IOPar& par )
{
    const IOObj* setupioobj = setupsel_->ioobj(true);
    PtrMan<IOObj> outputioobj = outputsel_->getIOObj(true);
    if ( !setupioobj || !outputioobj )
	return false; 

    par.set( VolProcessingTranslatorGroup::sKeyChainID(), setupioobj->key() );
    possubsel_->fillPar( par );

    par.set( VolProcessingTranslatorGroup::sKeyOutputID(), outputioobj->key() );
    return true;
}


void VolProc::uiBatchSetup::setupSelCB( CallBacker* )
{
    const IOObj* outputioobj = setupsel_->ioobj(true);
    editsetup_->setText( outputioobj ? "Edit ..." : "Create ..." );
}
