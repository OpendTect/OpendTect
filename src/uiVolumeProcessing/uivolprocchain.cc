/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivolprocchain.cc,v 1.1 2008-02-25 19:14:55 cvskris Exp $";

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "volprocchain.h"
#include "volproctrans.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimenu.h"


namespace VolProc
{

mImplFactory2Param( uiDialog, uiParent*, Step*, uiPS );



uiChain::uiChain( uiParent* p, Chain& man )
    : uiDialog( p, uiDialog::Setup("Volume Processing Setup",0,"dgb:104.1.3")
	    .savetext("Save on OK").savebutton(true).savechecked(true)
	    .menubar(true) )
    , chain_( man )
{
    setCancelText( 0 );

    uiPopupMenu* filemnu = new uiPopupMenu(this, "File" );
    newmenu_ = new uiMenuItem( "New",
	    mCB(this,uiChain,newButtonCB) );
    filemnu->insertItem( newmenu_ );
    newmenu_->setEnabled(false);  //No impl yet

    loadmenu_ = new uiMenuItem( "Load",
	    mCB(this,uiChain,loadButtonCB) );
    filemnu->insertItem( loadmenu_ );

    savemenu_ = new uiMenuItem( "Save",
	    mCB(this,uiChain,saveButtonCB) );
    filemnu->insertItem( savemenu_ );

    saveasmenu_ = new uiMenuItem( "Save As",
	    mCB(this,uiChain,saveAsButtonCB) );
    filemnu->insertItem( saveasmenu_ );

    menuBar()->insertItem( filemnu );

    uiLabel* availablelabel = new uiLabel( this, "Available steps" );
    factorylist_ = new uiListBox( this, PS().getNames(true) );
    factorylist_->selectionChanged.notify(
	    mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );

    addstepbutton_ = new uiPushButton( this, "Add",
	    mCB(this,uiChain,addStepCB), true );
    addstepbutton_->attach( rightOf, factorylist_ );

    removestepbutton_ = new uiPushButton( this, "Remove",
	    mCB(this,uiChain,removeStepCB), true);
    removestepbutton_->attach( alignedBelow, addstepbutton_ );

    steplist_ = new uiListBox( this );
    steplist_->attach( rightOf, addstepbutton_ );
    steplist_->selectionChanged.notify(
	    mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify(
	    mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( this, "Used steps" );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiPushButton( this, "Move Up",
	    mCB(this,uiChain,moveUpCB), true );
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiPushButton( this, "Move Down",
	    mCB(this,uiChain,moveDownCB), true);
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiPushButton( this, "Properties",
	    mCB(this,uiChain,propertiesCB), false );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    updateList();
    updateButtons();
}


bool uiChain::acceptOK(CallBacker*)
{
    if ( saveButtonChecked() )
	return doSave();

    return true;
}


bool uiChain::doSave()
{
    PtrMan<IOObj> ioobj = IOM().get( chain_.storageID() );
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
     CtxtIOObj context( VolProcessingTranslatorGroup::ioContext(), 0 );
     context.ctxt.forread = false;
     uiIOObjSelDlg dlg( this, context );
     if ( !dlg.go() || !dlg.nrSel() )
	 return false;

     BufferString errmsg;
     if ( VolProcessingTranslator::store( chain_, dlg.ioObj(), errmsg ) )
     {
	 chain_.setStorageID( dlg.ioObj()->key() );
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
	const char* key = chain_.getStep(idx)->type();
	const int keyidx = PS().getNames(false).indexOf( key );
	if ( idx>=steplist_->size() )
	    steplist_->addItem( PS().getNames(true)[keyidx]->buf(), false);
	else
	    steplist_->setItemText( idx, PS().getNames(true)[keyidx]->buf() );
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

    propertiesbutton_->setSensitive( stepsel!=-1 &&
	    hasPropDialog(stepsel) );
}


bool uiChain::hasPropDialog(int idx) const
{
    const Step* step = chain_.getStep( idx );
    if ( !step ) return false;

    return uiPS().getNames(false).indexOf( step->type() )!=-1;
}


void uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return;

    PtrMan<uiDialog> dlg = uiPS().create( step->type(), this, step );

    if ( !dlg )
	return;

    dlg->go();
}


void uiChain::loadButtonCB( CallBacker* )
{
     CtxtIOObj context( VolProcessingTranslatorGroup::ioContext(), 0 );
     IOPar* par = new IOPar;
     par->setYN( VolProcessingTranslatorGroup::sKeyIsVolProcSetup(),
	         getYesNoString(true));
     context.setPar( par );
     uiIOObjSelDlg dlg( this, context );
     if ( !dlg.go() || !dlg.nrSel() )
	 return;

     BufferString errmsg;
     if ( VolProcessingTranslator::retrieve( chain_, dlg.ioObj(), errmsg ) )
     {
	 chain_.setStorageID( dlg.ioObj()->key() );
	 updateList();
	 return;
     }

     updateList();
     uiMSG().error(errmsg);
}


void uiChain::saveButtonCB(CallBacker* cb)
{
    doSave();
}


void uiChain::saveAsButtonCB(CallBacker*)
{ doSaveAs(); }



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


void uiChain::addStepCB(CallBacker*)
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


void uiChain::removeStepCB(CallBacker*)
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
