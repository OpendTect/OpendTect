/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessor.h"

#include "ioman.h"
#include "ioobj.h"
#include "prestackprocessor.h"
#include "prestackprocessortransl.h"
#include "ptrman.h"

#include "uibuttongroup.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiprestackprocessorsel.h"


namespace PreStack
{

mImplFactory2Param( uiDialog, uiParent*, Processor*, uiPSPD )


uiProcessorManager::uiProcessorManager( uiParent* p, ProcessManager& man,
					int openidx,
					const uiStringSet* usemethods )
    : uiGroup(p)
    , manager_(man)
    , openidx_(openidx)
    , change(this)
{
    manager_.fillPar( restorepar_ );

    const uiString lbltxt = tr("Preprocessing methods");
    auto* label = new uiLabel( this, lbltxt );

    uiStringSet methods;
    if ( usemethods && !usemethods->isEmpty() )
	methods = *usemethods;
    else
	getMethods( manager_.getGeomSystem(), methods );

    factorylist_ = new uiListBox( this, "Preprocessing methods",
				  OD::ChooseOnlyOne );
    factorylist_->addItems( methods );
    factorylist_->resizeHeightToContents();
    factorylist_->setHSzPol( uiObject::Wide );
    mAttachCB( factorylist_->selectionChanged,
	       uiProcessorManager::factoryClickCB );
    mAttachCB( factorylist_->doubleClicked,
	       uiProcessorManager::factoryDoubleClickCB );
    factorylist_->attach( ensureBelow, label );

    addprocessorbutton_ = new uiToolButton( this, uiToolButton::RightArrow,
			      uiStrings::phrAdd(tr("method")),
			      mCB(this,uiProcessorManager,addCB) );
    addprocessorbutton_->attach( centeredRightOf, factorylist_ );

    processorlist_ = new uiListBox( this );
    processorlist_->setHSzPol( uiObject::Wide );
    processorlist_->attach( rightTo, factorylist_ );
    processorlist_->attach( ensureRightOf, addprocessorbutton_ );
    mAttachCB( processorlist_->selectionChanged,
	       uiProcessorManager::processorClickCB );
    mAttachCB( processorlist_->doubleClicked,
	       uiProcessorManager::processorDoubleClickCB );

    label = new uiLabel( this, tr("Used preprocessing methods") );
    label->attach( alignedAbove, processorlist_ );

    auto* butgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    butgrp->attach( rightOf, processorlist_ );
    moveupbutton_ = new uiToolButton( butgrp, uiToolButton::UpArrow,
		uiStrings::sMoveUp(), mCB(this,uiProcessorManager,moveUpCB) );

    movedownbutton_ = new uiToolButton( butgrp, uiToolButton::DownArrow,
	       uiStrings::sMoveDown(), mCB(this,uiProcessorManager,moveDownCB));

    propertiesbutton_ = new uiToolButton( butgrp, "settings",
				    uiStrings::phrEdit(
				    uiStrings::sStep().toLower()),
				    mCB(this,uiProcessorManager,propertiesCB) );

    removeprocessorbutton_ = new uiToolButton( butgrp, "trashcan",
			     uiStrings::phrRemove(uiStrings::sStep().toLower()),
			     mCB(this,uiProcessorManager,removeCB) );

    auto* iogrp = new uiButtonGroup( this, "IO Buttons", OD::Horizontal );
    iogrp->attach( alignedBelow, factorylist_ );
    iogrp->attach( ensureBelow, processorlist_ );
    loadbutton_ = new uiToolButton( iogrp, "open", tr("Open stored setup"),
				mCB(this, uiProcessorManager,loadCB) );
    savebutton_ = new uiToolButton( iogrp, "save", tr("Save setup"),
				mCB(this, uiProcessorManager,saveCB) );
    saveasbutton_ = new uiToolButton( iogrp, "saveas", tr("Save setup as"),
				mCB(this, uiProcessorManager,saveAsCB) );

    mAttachCB( postFinalize(), uiProcessorManager::initGrpCB );
}


uiProcessorManager::~uiProcessorManager()
{
    detachAllNotifiers();
}


void uiProcessorManager::initGrpCB( CallBacker* )
{
    updateList();
    updateButtons();
    if ( factorylist_->validIdx(openidx_) )
    {
	factorylist_->setCurrentItem( openidx_ );
	addCB( nullptr );
    }
}


void uiProcessorManager::getMethods( OD::GeomSystem gs, uiStringSet& nms )
{
    const bool issynth = ::isSynthetic( gs );
    const BufferStringSet& methods = Processor::factory().getNames();
    for ( const auto* method : methods )
    {
	PtrMan<Processor> proc = Processor::factory().create( method->str() );
	if ( !proc || !proc->showInEditor() ||
	     (issynth && !proc->canDoSynthetics()) )
	    continue;

	nms.add( proc->factoryDisplayName() );
    }
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
    for ( ; idx<manager_.nrProcessors(); idx++ )
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


bool uiProcessorManager::hasPropDialog( int idx ) const
{
    const Processor* proc = manager_.getProcessor(idx);
    if ( !proc )
	return false;

    return uiPSPD().getNames().isPresent( proc->name() );
}


bool uiProcessorManager::showPropDialog( int idx )
{
    if ( idx < 0 || idx >= manager_.nrProcessors() )
	return false;

    Processor* proc = manager_.getProcessor(idx);
    return proc ? showPropDialog( *proc ) : false;
}


bool uiProcessorManager::showPropDialog( Processor& proc )
{
    PtrMan<uiDialog> dlg = uiPSPD().create( proc.name(), this, &proc );
    if ( !dlg || !dlg->go() )
	return false;

    change.trigger();
    manager_.notifyChange();
    changed_ = true;
    updateButtons();
    return true;
}


void uiProcessorManager::factoryClickCB( CallBacker* )
{
    if ( factorylist_->firstChosen()==-1 )
	return;

    processorlist_->chooseAll( false );
    updateButtons();
}


void uiProcessorManager::factoryDoubleClickCB( CallBacker* cb )
{
    addCB( cb );
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


void uiProcessorManager::addCB( CallBacker* )
{
    if ( factorylist_->firstChosen()==-1 )
	return;

    const FactoryBase& fact = Processor::factory();
    const uiString method = ::toUiString( factorylist_->getText() );
    if ( !fact.getUserNames().isPresent(method) )
	return;

    const int idx = fact.getUserNames().indexOf( method );
    if ( !fact.getNames().validIdx(idx) )
	return;

    const char* nm = fact.getNames().get( idx ).buf();
    Processor* proc = Processor::factory().create( nm );
    if ( !proc )
	return;

    proc->setGeomSystem( manager_.getGeomSystem() );
    manager_.addProcessor( proc );
    updateList();
    if ( proc->mustHaveUserInput() && !showPropDialog(*proc) )
    {
	const int iproc = manager_.indexOf( proc );
	manager_.removeProcessor( iproc );
	updateList();
	return;
    }

    processorlist_->chooseAll( false );
    change.trigger();
    changed_ = true;
    updateButtons();
}


void uiProcessorManager::removeCB( CallBacker* )
{
    const int idx = processorlist_->firstChosen();
    if ( idx<0 )
	return;

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
    if ( idx<1 )
	return;

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
    const IOObjContext& ctxt = uiPSProcObjSel::ioContext(
						manager_.getGeomSystem() );
    uiIOObjSelDlg dlg( this, ctxt );
    if ( dlg.go() && dlg.ioObj() )
    {
	uiString errmsg;
	if ( !PreStackProcTranslator::retrieve(manager_,dlg.ioObj(),errmsg) )
	{
	    uiMSG().error( errmsg );
	    return;
	}

	updateList();
	lastmid_ = dlg.ioObj()->key();
    }

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
    IOObjContext ctxt = uiPSProcObjSel::ioContext( manager_.getGeomSystem() );
    ctxt.forread_ = false;
    uiIOObjSelDlg dlg( this, ctxt );
    if ( dlg.go() )
    {
	const IOObj* selobj = dlg.ioObj();
	if (selobj && doSave(*selobj) )
	{
	    lastmid_ = selobj->key();
	    return true;
	}
    }

    return false;
}


void uiProcessorManager::saveAsCB( CallBacker* )
{
    doSaveAs();
}


void uiProcessorManager::saveCB( CallBacker* )
{
    PtrMan<IOObj> ioobj = IOM().get( lastmid_ );
    if ( ioobj )
	doSave( *ioobj );
    else
	doSaveAs();
}


bool uiProcessorManager::doSave( const IOObj& ioobj )
{
    uiString errmsg;
    if ( !PreStackProcTranslator::store(manager_,&ioobj,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    changed_ = false;
    updateButtons();
    return true;
}

} // namespace PreStack
