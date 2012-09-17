/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocchain.cc,v 1.27 2011/11/02 14:46:05 cvsnanne Exp $";

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "settings.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "uitoolbutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "uiseparator.h"


namespace VolProc
{


const char* uiChain::sKeySettingKey()
{ return "dTect.ProcessVolumeBuilderOnOK"; }   

mImplFactory2Param( uiStepDialog, uiParent*, Step*, uiStepDialog::factory );


uiStepDialog::uiStepDialog( uiParent* p, const char* stepnm, Step* step )
    : uiDialog( p, uiDialog::Setup("Edit step",stepnm,mNoHelpID) )
    , step_(step)
{
}


void uiStepDialog::addNameFld( uiObject* alignobj )
{
    uiSeparator* sep = 0;
    if ( alignobj )
    {
	sep = new uiSeparator( this, "namefld sep" );
	sep->attach( stretchedBelow, alignobj );
    }
    namefld_ = new uiGenInput( this, "Name for this step", step_->userName() );
    if ( alignobj )
    {
	namefld_->attach( ensureBelow, sep );
	namefld_->attach( alignedBelow, alignobj );
    }
}


void uiStepDialog::addNameFld( uiGroup* aligngrp )
{
    uiObject* alignobj = aligngrp ? aligngrp->attachObj() : 0;
    addNameFld( alignobj );
}


bool uiStepDialog::acceptOK( CallBacker* )
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name for this step" );
	return false;
    }

    step_->setUserName( nm.buf() );

    return true;
}


uiChain::uiChain( uiParent* p, Chain& chn, bool withprocessnow )
    : uiDialog( p, uiDialog::Setup("Volume Builder: Setup",0,"103.6.0")
	    .menubar(true) )
    , chain_(chn)
{
    uiToolBar* tb = new uiToolBar( this, "Load/Save toolbar", uiToolBar::Right);
    tb->addButton( "open.png", "Read stored setup", mCB(this,uiChain,readPush));
    tb->addButton( "save.png", "Save setup now", mCB(this,uiChain,savePush) );

    uiGroup* flowgrp = new uiGroup( this, "Flow group" );

    const CallBack addcb( mCB(this,uiChain,addStepPush) );
    uiLabel* availablelabel = new uiLabel( flowgrp, "Available steps" );
    factorylist_ = new uiListBox( flowgrp,
				  uiStepDialog::factory().getNames(true) );
    factorylist_->setHSzPol( uiObject::Wide );
    factorylist_->selectionChanged.notify( mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );
    factorylist_->doubleClicked.notify( addcb );

    addstepbutton_ = new uiToolButton( flowgrp, uiToolButton::RightArrow,
					"Add step", addcb );
    addstepbutton_->attach( centeredRightOf, factorylist_ );

    steplist_ = new uiListBox( flowgrp );
    steplist_->setHSzPol( uiObject::Wide );
    steplist_->attach( rightTo, factorylist_ );
    steplist_->attach( ensureRightOf, addstepbutton_ );
    steplist_->selectionChanged.notify( mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify( mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( flowgrp, "Used steps" );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiToolButton( flowgrp, uiToolButton::UpArrow,
				"Move step up", mCB(this,uiChain,moveUpCB) );
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiToolButton( flowgrp, uiToolButton::DownArrow,
			    "Move step down", mCB(this,uiChain,moveDownCB) );
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiToolButton( flowgrp, "settings.png",
	    				  "Edit this step",
					  mCB(this,uiChain,propertiesCB) );
    propertiesbutton_->setName( "Settings" );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    removestepbutton_ = new uiToolButton( flowgrp, "trashcan.png",
	    	"Remove step from flow", mCB(this,uiChain,removeStepPush) );
    removestepbutton_->attach( alignedBelow, propertiesbutton_ );

    flowgrp->setHAlignObj( steplist_ );

    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = false;

    objfld_ = new uiIOObjSel( this, ctxt, "On OK, store As" );
    objfld_->setInput( chain_.storageID() );
    objfld_->setConfirmOverwrite( false );
    objfld_->attach( alignedBelow, flowgrp );

    if ( withprocessnow )
    {
	enableSaveButton( "Process on OK" );
	bool enabled = false;
	Settings::common().getYN( sKeySettingKey(), enabled );
	setSaveButtonChecked( enabled );
    }

    updateList();
    updateButtons();
}


uiChain::~uiChain()
{
}


void uiChain::updObj( const IOObj& ioobj )
{
    chain_.setStorageID( ioobj.key() );
    objfld_->setInput( ioobj.key() );
}


const MultiID& uiChain::storageID() const
{
    return chain_.storageID();
}


bool uiChain::acceptOK(CallBacker*)
{
    if ( chain_.nrSteps() && chain_.getStep( 0 ) &&
       chain_.getStep( 0 )->needsInput() )
    {
	if ( !uiMSG().askGoOn("The first step in the chain needs an input, "
                  "and can thus not be first. Proceed anyway?", true ) )
	    return false;
    }

    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
    {
	uiMSG().error("Please enter a name for the setup");
	return false;
    }

    chain_.setStorageID( ioobj->key() );
    if ( hasSaveButton() )
    {
	Settings::common().setYN( sKeySettingKey(), saveButtonChecked() );
	Settings::common().write();
    }

    return doSave();
}


bool uiChain::doSave()
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
	return doSaveAs();

    BufferString errmsg;
    if ( VolProcessingTranslator::store( chain_, ioobj, errmsg ) )
	return true;

     uiMSG().error( errmsg );
     return false;
}


bool uiChain::doSaveAs()
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    uiIOObjSelDlg dlg( this, ctxt, "Volume Builder Setup" );
    if ( !dlg.go() || !dlg.nrSel() )
	 return false;

     BufferString errmsg;
     if ( VolProcessingTranslator::store( chain_, dlg.ioObj(), errmsg ) )
     {
	 updObj( *dlg.ioObj() );
	 return true;
     }

     uiMSG().error( errmsg );
     return false;
}


void uiChain::updateList()
{
    NotifyStopper stopper( steplist_->selectionChanged );
    int idx=0;
    for ( ; idx<chain_.nrSteps(); idx ++ )
    {
	const char* key = chain_.getStep(idx)->factoryKeyword();
	const char* username = chain_.getStep(idx)->userName();
	const char* displayname = username;
	if ( !displayname )
	{
	    displayname = chain_.getStep(idx)->factoryDisplayName();
	    if ( !displayname )
		displayname = key;
	}

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

    const bool hasdlg = stepsel>=0 &&
	uiStepDialog::factory().hasName(
		chain_.getStep(stepsel)->factoryKeyword());

    propertiesbutton_->setSensitive( hasdlg );
}


bool uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return false;

    PtrMan<uiStepDialog> dlg = uiStepDialog::factory().create(
	    step->factoryKeyword(), this, step );
    if ( !dlg )
    {
	uiMSG().error( "Internal error. Step cannot be created" );
	return false;
    }

    bool ret = dlg->go();
    if ( ret )
	updateList();

    return ret;
}


void uiChain::readPush( CallBacker* )
{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = true;
    uiIOObjSelDlg dlg( this, ctxt );
    dlg.selGrp()->setConfirmOverwrite( false );
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

    const char* steptype = uiStepDialog::factory().getNames(false)[sel]->buf();
    Step* step = Step::factory().create( steptype );
    if ( !step ) return;

    chain_.addStep( step );
    updateList();
    steplist_->selectAll( false );
    int curitm = steplist_->size() - 1;
    steplist_->setCurrentItem( curitm );
    if ( !showPropDialog(curitm) )
    {
	chain_.removeStep( curitm );
	updateList();
	curitm--;
	if ( curitm >= 0 )
	    steplist_->setCurrentItem( curitm );
    }
    updateButtons();
}


void uiChain::removeStepPush(CallBacker*)
{
    int curitm = steplist_->nextSelected(-1);
    if ( curitm < 0 )
	return;

    chain_.removeStep( curitm );
    updateList();
    steplist_->selectAll(false);
    if ( curitm >= steplist_->size() )
	curitm--;
    if ( curitm >= 0 )
	steplist_->setCurrentItem( curitm );

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
