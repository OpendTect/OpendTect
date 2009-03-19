/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocchain.cc,v 1.8 2009-03-19 13:27:12 cvsbert Exp $";

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitoolbar.h"


namespace VolProc
{

mImplFactory2Param( uiStepDialog, uiParent*, Step*, uiChain::factory );


uiStepDialog::uiStepDialog(uiParent* p,const uiDialog::Setup& s,Step* step)
    : uiDialog( p, s )
    , step_( step )
{
    const char* key = step_->type();
    const char* username = step_->userName();
    const int keyidx = PS().getNames(false).indexOf( key );
    const char* displayname = username
	? username
	: PS().getNames(true)[keyidx]->buf();

    namefld_ = new uiGenInput( this, sKey::Name,
	    		       StringInpSpec( displayname ) );
}


bool uiStepDialog::acceptOK( CallBacker* )
{
    const char* key = step_->type();
    const int keyidx = PS().getNames(false).indexOf( key );

    BufferString nm = namefld_->text();
    if ( nm==PS().getNames(true)[keyidx]->buf() )
	step_->setUserName( 0 );
    else
	step_->setUserName( nm.buf() );

    return true;
}


uiChain::uiChain( uiParent* p, Chain& chn )
    : uiDialog( p, uiDialog::Setup("Volume Builder Setup",0,"dgb:104.0.3")
	    .menubar(true) )
    , chain_(chn)
    , ctio_(*mMkCtxtIOObj(VolProcessing))
{
    ctio_.ctxt.forread = false;
    ctio_.setObj( IOM().get(chain_.storageID()) );

    uiToolBar* tb = new uiToolBar( this, "Load/Save toolbar", uiToolBar::Right);
    tb->addButton( "openflow.png", mCB(this,uiChain,readPush),
	    	   "Read builder setup", false );
    tb->addButton( "saveflow.png", mCB(this,uiChain,savePush),
	    	   "Save builder setup", false );

    uiGroup* flowgrp = new uiGroup( this, "Flow group" );

    uiLabel* availablelabel = new uiLabel( flowgrp, "Available steps" );
    factorylist_ = new uiListBox( flowgrp, PS().getNames(true) );
    factorylist_->selectionChanged.notify(
	    mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );

    addstepbutton_ = new uiPushButton( flowgrp, "Add",
	    mCB(this,uiChain,addStepPush), true );
    addstepbutton_->attach( rightOf, factorylist_ );
    flowgrp->setHAlignObj( addstepbutton_ );

    removestepbutton_ = new uiPushButton( flowgrp, "Remove",
	    mCB(this,uiChain,removeStepPush), true);
    removestepbutton_->attach( alignedBelow, addstepbutton_ );

    steplist_ = new uiListBox( flowgrp );
    steplist_->attach( rightOf, addstepbutton_ );
    steplist_->selectionChanged.notify(
	    mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify(
	    mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( flowgrp, "Used steps" );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiPushButton( flowgrp, "Move Up",
	    mCB(this,uiChain,moveUpCB), true );
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiPushButton( flowgrp, "Move Down",
	    mCB(this,uiChain,moveDownCB), true);
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiPushButton( flowgrp, "Settings",
	    mCB(this,uiChain,propertiesCB), false );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    objfld_ = new uiIOObjSel( this, ctio_, "Volume Builder Setup" );
    objfld_->setConfirmOverwrite( false );
    objfld_->attach( alignedBelow, flowgrp );

    updateList();
    updateButtons();
}


uiChain::~uiChain()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiChain::updObj( const IOObj& ioobj )
{
    chain_.setStorageID( ioobj.key() );
    ctio_.setObj( ioobj.clone() );
    objfld_->updateInput();
}


const MultiID& uiChain::storageID() const
{
    return chain_.storageID();
}


bool uiChain::acceptOK(CallBacker*)
{
    if ( !objfld_->commitInput(true) )
    {
	uiMSG().error( "Please enter a name for this builder setup" );
	return false;
    }

    chain_.setStorageID( ctio_.ioobj->key() );
    return doSave();
}


bool uiChain::doSave()
{
    if ( !ctio_.ioobj )
	return doSaveAs();

    BufferString errmsg;
    if ( VolProcessingTranslator::store( chain_, ctio_.ioobj, errmsg ) )
	return true;

     uiMSG().error( errmsg );
     return false;
}


bool uiChain::doSaveAs()
{
     uiIOObjSelDlg dlg( this, ctio_, "Volume Builder Setup" );
     if ( !dlg.go() || !dlg.nrSel() )
	 return false;

     BufferString errmsg;
     if ( VolProcessingTranslator::store( chain_, dlg.ioObj(), errmsg ) )
     {
	 updObj( *dlg.ioObj() );
	 return true;
     }

     ctio_.setObj( IOM().get(chain_.storageID()) );
     uiMSG().error( errmsg );
     return false;
}


void uiChain::updateList()
{
    NotifyStopper stopper( steplist_->selectionChanged );
    int idx=0;
    for ( ; idx<chain_.nrSteps(); idx ++ )
    {
	const char* key = chain_.getStep(idx)->type();
	const char* username = chain_.getStep(idx)->userName();
	const int keyidx = PS().getNames(false).indexOf( key );
	const char* displayname = username
	    ? username
	    : PS().getNames(true)[keyidx]->buf();

	if ( idx>=steplist_->size() )
	    steplist_->addItem( displayname, false);
	else
	    steplist_->setItemText( idx, displayname );
    }

    for ( ; idx<steplist_->size(); )
	steplist_->removeItem( idx );
}


void uiChain::updateButtons()
{
    const bool factoryselected = factorylist_->nextSelected(-1)!=-1;
    const int stepsel = steplist_->nextSelected(-1);

    addstepbutton_->setSensitive( factoryselected );
    removestepbutton_->setSensitive( stepsel!=-1 );

    moveupbutton_->setSensitive( stepsel>0 );
    movedownbutton_->setSensitive( stepsel!=-1 &&
	    stepsel!=steplist_->size()-1 );

    propertiesbutton_->setSensitive( stepsel!=-1 );
}


void uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return;

    PtrMan<uiStepDialog> dlg = factory().create( step->type(), this, step );

    if ( !dlg )
    {
	mTryAlloc( dlg, uiStepDialog( this,
		    uiDialog::Setup("Select name","Select name",mNoHelpID),
		    	step ) );
	if ( !dlg )
	    return;
    }

    if ( dlg->go() )
	updateList();

}


void uiChain::readPush( CallBacker* )
{
     IOPar* par = new IOPar;
     par->setYN( VolProcessingTranslatorGroup::sKeyIsVolProcSetup(),
	         toString(true));
     ctio_.setPar( par );
     uiIOObjSelDlg dlg( this, ctio_ );
     if ( !dlg.go() || !dlg.nrSel() )
	 return;

     BufferString errmsg;
     if ( VolProcessingTranslator::retrieve( chain_, dlg.ioObj(), errmsg ) )
     {
	 updObj( *dlg.ioObj() );
	 updateList();
	 return;
     }

     updateList();
     uiMSG().error(errmsg);
}


void uiChain::savePush(CallBacker* cb)
{
    doSaveAs();
}


void uiChain::factoryClickCB(CallBacker*)
{
    if ( factorylist_->nextSelected(-1) == -1 )
	return;

    steplist_->selectAll(false);
    updateButtons();
}


void uiChain::stepClickCB(CallBacker*)
{
    if ( steplist_->nextSelected(-1) == -1 )
	return;

    factorylist_->selectAll(false);
    updateButtons();
}


void uiChain::stepDoubleClickCB(CallBacker*)
{
    factorylist_->selectAll(false);
    updateButtons();

    showPropDialog( steplist_->currentItem() );
}


void uiChain::addStepPush(CallBacker*)
{
    const int sel = factorylist_->nextSelected(-1);
    if ( sel == -1 )
	return;

    const char* steptype = PS().getNames(false)[sel]->buf();
    Step* step = PS().create( steptype, chain_ );
    if ( !step ) return;

    chain_.addStep( step );
    updateList();
    steplist_->selectAll(false);
    updateButtons();
}


void uiChain::removeStepPush(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 )
	return;

    chain_.removeStep( idx );
    updateList();
    steplist_->selectAll(false);

    updateButtons();
}


void uiChain::moveUpCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);

    if ( idx<1 ) return;

    chain_.swapSteps( idx, idx-1 );
    updateList();

    steplist_->setSelected( idx-1, true );
    updateButtons();
}


void uiChain::moveDownCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 || idx>=chain_.nrSteps() )
	return;

    chain_.swapSteps( idx, idx+1 );
    updateList();
    steplist_->setSelected( idx+1, true );
    updateButtons();
}


void uiChain::propertiesCB(CallBacker*)
{
    const int idx = steplist_->nextSelected(-1);
    if ( idx<0 ) return;

    showPropDialog( idx );
}

}; //namespace
