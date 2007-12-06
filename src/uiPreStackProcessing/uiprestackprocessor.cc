/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackprocessor.cc,v 1.7 2007-12-06 20:05:45 cvskris Exp $";

#include "uiprestackprocessor.h"

#include "ptrman.h"
#include "prestackprocessor.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uilistbox.h"


namespace PreStack
{

mImplFactory2Param( uiDialog, uiParent*, Processor*, uiPSPD );


uiProcessorManager::uiProcessorManager( uiParent* p, ProcessManager& man )
    : uiGroup( p )
    , manager_( man )
    , change( this )
{
    manager_.fillPar( restorepar_ );

    uiLabel* label = new uiLabel( this, "Preprocessing methods" );

    factorylist_ = new uiListBox( this, PF().getNames() );
    factorylist_->selectionChanged.notify(
	    mCB(this,uiProcessorManager,factoryClickCB) );
    factorylist_->attach( ensureBelow, label );

    addprocessorbutton_ = new uiPushButton( this, "Add",
	    mCB(this,uiProcessorManager,addProcessorCB), true );
    addprocessorbutton_->attach( rightOf, factorylist_ );

    removeprocessorbutton_ = new uiPushButton( this, "Remove",
	    mCB(this,uiProcessorManager,removeProcessorCB), true);
    removeprocessorbutton_->attach( alignedBelow, addprocessorbutton_ );

    processorlist_ = new uiListBox( this );
    processorlist_->attach( rightOf, addprocessorbutton_ );
    processorlist_->selectionChanged.notify(
	    mCB(this,uiProcessorManager,processorClickCB) );
    processorlist_->doubleClicked.notify(
	    mCB(this,uiProcessorManager,processorDoubleClickCB) );

    label = new uiLabel( this, "Used preprocessing methods" );
    label->attach( alignedAbove, processorlist_ );

    moveupbutton_ = new uiPushButton( this, "Move Up",
	    mCB(this,uiProcessorManager,moveUpCB), true );
    moveupbutton_->attach( rightOf, processorlist_ );

    movedownbutton_ = new uiPushButton( this, "Move Down",
	    mCB(this,uiProcessorManager,moveDownCB), true);
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiPushButton( this, "Properties",
	    mCB(this,uiProcessorManager,propertiesCB), false );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    updateList();
    updateButtons();
}


bool uiProcessorManager::restore()
{
    return manager_.usePar( restorepar_ );
}


void uiProcessorManager::updateList()
{
    NotifyStopper stopper( processorlist_->selectionChanged );
    int idx=0;
    for ( ; idx<manager_.nrProcessors(); idx ++ )
    {
	const char* text =  manager_.getProcessor(idx)->name();
	if ( idx>=processorlist_->size() )
	    processorlist_->addItem( text, false);
	else
	    processorlist_->setItemText( idx, text );
    }

    for ( ; idx<processorlist_->size(); )
	processorlist_->removeItem( idx );
}


void uiProcessorManager::updateButtons()
{
    const bool factoryselected = factorylist_->nextSelected(-1)!=-1;
    const int processorsel = processorlist_->nextSelected(-1);

    addprocessorbutton_->setSensitive( factoryselected );
    removeprocessorbutton_->setSensitive( processorsel!=-1 );

    moveupbutton_->setSensitive( processorsel>0 );
    movedownbutton_->setSensitive( processorsel!=-1 &&
	    processorsel!=processorlist_->size()-1 );

    propertiesbutton_->setSensitive( processorsel!=-1 &&
	    hasPropDialog(processorsel) );
}


bool uiProcessorManager::hasPropDialog(int idx) const
{
    const Processor* proc = manager_.getProcessor(idx);
    if ( !proc ) return false;

    return uiPSPD().getNames().indexOf( proc->name() )!=-1;
}


void uiProcessorManager::showPropDialog( int idx )
{
    Processor* proc = manager_.getProcessor(idx);
    if ( !proc ) return;

    PtrMan<uiDialog> dlg = uiPSPD().create( proc->name(), this, proc );

    if ( !dlg ) return;

    if ( dlg->go() )
    {
	change.trigger();
	manager_.notifyChange();
    }
	
}


void uiProcessorManager::factoryClickCB( CallBacker* )
{
    if ( factorylist_->nextSelected(-1)==-1 )
	return;

    processorlist_->selectAll(false);
    updateButtons();
}


void uiProcessorManager::processorClickCB( CallBacker* )
{
    if ( processorlist_->nextSelected(-1)==-1 )
	return;

    factorylist_->selectAll( false );
    updateButtons();
}


void uiProcessorManager::processorDoubleClickCB( CallBacker* )
{
    factorylist_->selectAll( false );
    updateButtons();

    showPropDialog( processorlist_->currentItem() );
}


void uiProcessorManager::addProcessorCB( CallBacker* )
{
    if ( factorylist_->nextSelected(-1)==-1 )
	return;

    Processor* proc = PF().create( factorylist_->getText() );
    if ( !proc ) return;

    manager_.addProcessor( proc );
    updateList();
    processorlist_->selectAll(false);
    updateButtons();
    change.trigger();
}


void uiProcessorManager::removeProcessorCB( CallBacker* )
{
    const int idx = processorlist_->nextSelected(-1);
    if ( idx<0 ) return;

    manager_.removeProcessor( idx );
    updateList();
    processorlist_->selectAll(false);

    updateButtons();
    change.trigger();
}


void uiProcessorManager::moveUpCB( CallBacker* )
{
    const int idx = processorlist_->nextSelected(-1);
    if ( idx<1 ) return;

    manager_.swapProcessors( idx, idx-1 );
    updateList();
    processorlist_->setSelected( idx-1, true );

    updateButtons();
    change.trigger();
}


void uiProcessorManager::moveDownCB( CallBacker* )
{
    const int idx = processorlist_->nextSelected(-1);
    if ( idx<0 || idx>=manager_.nrProcessors() )
	return;

    manager_.swapProcessors( idx, idx+1 );
    updateList();
    processorlist_->setSelected( idx+1, true );
    updateButtons();
    change.trigger();
}


void uiProcessorManager::propertiesCB( CallBacker* )
{
    const int idx = processorlist_->nextSelected(-1);
    if ( idx<0 ) return;

    showPropDialog( idx );
}

}; //namespace
