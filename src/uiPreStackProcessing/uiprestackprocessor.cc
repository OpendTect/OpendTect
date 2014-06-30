/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackprocessor.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "prestackprocessor.h"
#include "prestackprocessortransl.h"
#include "uibutton.h"
#include "uiicons.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uistrings.h"
#include "uimsg.h"


namespace PreStack
{

mImplFactory2Param( uiDialog, uiParent*, Processor*, uiPSPD );


uiProcessorManager::uiProcessorManager( uiParent* p, ProcessManager& man )
    : uiGroup( p )
    , manager_( man )
    , change( this )
    , changed_( false )
{
    manager_.fillPar( restorepar_ );

    const char* lbltxt = "Preprocessing methods";
    uiLabel* label = new uiLabel( this, lbltxt );

    factorylist_ = new uiListBox( this, Processor::factory().getUserNames(),
				  lbltxt, OD::ChooseOnlyOne );
    factorylist_->selectionChanged.notify(
	    mCB(this,uiProcessorManager,factoryClickCB) );
    factorylist_->attach( ensureBelow, label );

    addprocessorbutton_ = new uiPushButton( this, uiStrings::sAdd(true),
	    mCB(this,uiProcessorManager,addProcessorCB), true );
    addprocessorbutton_->attach( rightOf, factorylist_ );

    removeprocessorbutton_ = new uiPushButton( this, uiStrings::sRemove(true),
	    mCB(this,uiProcessorManager,removeProcessorCB), true);
    removeprocessorbutton_->attach( alignedBelow, addprocessorbutton_ );

    processorlist_ = new uiListBox( this );
    processorlist_->attach( rightOf, addprocessorbutton_ );
    processorlist_->attach( heightSameAs, factorylist_ );
    processorlist_->selectionChanged.notify(
	    mCB(this,uiProcessorManager,processorClickCB) );
    processorlist_->doubleClicked.notify(
	    mCB(this,uiProcessorManager,processorDoubleClickCB) );

    label = new uiLabel( this, tr("Used preprocessing methods") );
    label->attach( alignedAbove, processorlist_ );

    moveupbutton_ = new uiPushButton( this, uiStrings::sUp(),
	    mCB(this,uiProcessorManager,moveUpCB), true );
    moveupbutton_->attach( rightOf, processorlist_ );

    movedownbutton_ = new uiPushButton( this, uiStrings::sDown(),
	    mCB(this,uiProcessorManager,moveDownCB), true);
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiPushButton( this, uiStrings::sProperties(true),
	    mCB(this,uiProcessorManager,propertiesCB), false );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    loadbutton_ = new uiPushButton( this, uiStrings::sLoad(true),
			ioPixmap(uiIcon::openObject()),
			mCB(this, uiProcessorManager,loadCB), true );
    loadbutton_->attach( alignedBelow, factorylist_ );

    savebutton_ = new uiPushButton( this, uiStrings::sSave(true),
			ioPixmap(uiIcon::save()),
	    mCB(this, uiProcessorManager,saveCB), true );
    savebutton_->attach( rightOf, loadbutton_ );

    saveasbutton_ = new uiPushButton( this, uiStrings::sSaveAs(true),
			    ioPixmap(uiIcon::saveAs()),
			    mCB(this, uiProcessorManager,saveAsCB), true );
    saveasbutton_->attach( rightOf, savebutton_ );

    updateList();
    updateButtons();
}


bool uiProcessorManager::restore()
{
    return manager_.usePar( restorepar_ );
}


void uiProcessorManager::setLastMid( const MultiID& mid )
{
    lastmid_ = mid;
}


void uiProcessorManager::updateList()
{
    NotifyStopper stopper( processorlist_->selectionChanged );
    int idx=0;
    for ( ; idx<manager_.nrProcessors(); idx ++ )
    {
	const char* procnm =  manager_.getProcessor(idx)->name();
	const int factoryidx =
	    Processor::factory().getNames().indexOf(procnm);
	const uiString& text =
	    Processor::factory().getUserNames()[factoryidx];

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
    const bool factoryselected = factorylist_->firstChosen() != -1;
    const int processorsel = processorlist_->firstChosen();

    addprocessorbutton_->setSensitive( factoryselected );
    removeprocessorbutton_->setSensitive( processorsel!=-1 );

    moveupbutton_->setSensitive( processorsel>0 );
    movedownbutton_->setSensitive( processorsel!=-1 &&
	    processorsel!=processorlist_->size()-1 );

    propertiesbutton_->setSensitive( processorsel!=-1 &&
	    hasPropDialog(processorsel) );

    saveasbutton_->setSensitive( manager_.nrProcessors() );
    savebutton_->setSensitive( changed_ );
}


bool uiProcessorManager::hasPropDialog(int idx) const
{
    const Processor* proc = manager_.getProcessor(idx);
    if ( !proc ) return false;

    return uiPSPD().getNames().isPresent( proc->name() );
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
	changed_ = true;
	updateButtons();
    }

}


void uiProcessorManager::factoryClickCB( CallBacker* )
{
    if ( factorylist_->firstChosen()==-1 )
	return;

    processorlist_->chooseAll(false);
    updateButtons();
}


void uiProcessorManager::processorClickCB( CallBacker* )
{
    if ( processorlist_->firstChosen()==-1 )
	return;

    factorylist_->chooseAll( false );
    updateButtons();
}


void uiProcessorManager::processorDoubleClickCB( CallBacker* )
{
    factorylist_->chooseAll( false );
    updateButtons();

    showPropDialog( processorlist_->currentItem() );
}


void uiProcessorManager::addProcessorCB( CallBacker* )
{
    if ( factorylist_->firstChosen()==-1 )
	return;

    const char* nm =
       Processor::factory().getNames()[factorylist_->currentItem()]->buf();
    Processor* proc = Processor::factory().create( nm );
    if ( !proc ) return;

    manager_.addProcessor( proc );
    updateList();
    processorlist_->chooseAll(false);
    change.trigger();
    changed_ = true;
    updateButtons();
}


void uiProcessorManager::removeProcessorCB( CallBacker* )
{
    const int idx = processorlist_->firstChosen();
    if ( idx<0 ) return;

    manager_.removeProcessor( idx );
    updateList();
    processorlist_->chooseAll(false);

    change.trigger();
    changed_ = true;
    updateButtons();
}


void uiProcessorManager::moveUpCB( CallBacker* )
{
    const int idx = processorlist_->firstChosen();
    if ( idx<1 ) return;

    manager_.swapProcessors( idx, idx-1 );
    updateList();
    processorlist_->setChosen( idx-1, true );

    change.trigger();
    changed_ = true;
    updateButtons();
}


void uiProcessorManager::moveDownCB( CallBacker* )
{
    const int idx = processorlist_->firstChosen();
    if ( idx<0 || idx>=manager_.nrProcessors() )
	return;

    manager_.swapProcessors( idx, idx+1 );
    updateList();
    processorlist_->setChosen( idx+1, true );
    change.trigger();
    changed_ = true;
    updateButtons();
}


void uiProcessorManager::propertiesCB( CallBacker* )
{
    const int idx = processorlist_->firstChosen();
    if ( idx<0 ) return;

    showPropDialog( idx );
}


void uiProcessorManager::loadCB( CallBacker* )
{
    CtxtIOObj selcontext = PreStackProcTranslatorGroup::ioContext();
    selcontext.ctxt.forread = true;

    uiIOObjSelDlg dlg( this, selcontext );
    if ( dlg.go() && dlg.ioObj() )
    {
	uiString errmsg;
	if ( !PreStackProcTranslator::retrieve( manager_, dlg.ioObj(), errmsg) )
	    uiMSG().error( errmsg );
	else
	{
	    updateList();
	    lastmid_ = dlg.ioObj()->key();
	}
    }

    delete selcontext.ioobj;
    changed_ = false;
    updateButtons();
    change.trigger();
}


bool uiProcessorManager::save()
{
    PtrMan<IOObj> ioobj = IOM().get( lastmid_ );
    if ( ioobj )
	return doSave( *ioobj );

    return doSaveAs();
}


bool uiProcessorManager::doSaveAs()
{
    CtxtIOObj selcontext = PreStackProcTranslatorGroup::ioContext();
    selcontext.ctxt.forread = false;

    uiIOObjSelDlg dlg( this, selcontext );
    if ( dlg.go() && dlg.ioObj() )
    {
	if ( doSave( *dlg.ioObj() ) )
	{
	    lastmid_ = dlg.ioObj()->key();
	    delete selcontext.ioobj;
	    return true;
	}
    }

    delete selcontext.ioobj;
    return false;
}


void uiProcessorManager::saveAsCB( CallBacker* )
{
    doSaveAs();
}


void uiProcessorManager::saveCB( CallBacker* )
{
    PtrMan<IOObj> ioobj = IOM().get( lastmid_ );
    if ( !ioobj )
	doSaveAs();
    else
	doSave( *ioobj );
}


bool uiProcessorManager::doSave( const IOObj& ioobj )
{
    uiString errmsg;
    if ( !PreStackProcTranslator::store( manager_, &ioobj, errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    changed_ = false;
    updateButtons();
    return true;
}

}; //namespace
