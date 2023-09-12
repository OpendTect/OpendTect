/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolprocchain.h"

#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "settings.h"
#include "volprocchain.h"
#include "volproctrans.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


namespace VolProc
{

const char* uiChain::sKeySettingKey()
{ return "dTect.ProcessVolumeBuilderOnOK"; }

mImplFactory3Param( uiStepDialog, uiParent*, Step*, bool,
		    uiStepDialog::factory );


// uiStepDialog
uiStepDialog::uiStepDialog( uiParent* p, const uiString& stepnm, Step* step,
			    bool is2d )
    : uiDialog( p, uiDialog::Setup(tr("Edit step"),stepnm,mNoHelpKey) )
    , step_(step)
    , multiinpfld_(0)
    , is2d_(is2d)
{
}


uiStepDialog::~uiStepDialog()
{}


void uiStepDialog::addMultiInputFld()
{
    addMultiInputFld( 0 );
}


void uiStepDialog::addMultiInputFld( uiGroup* grp )
{
    const int nrinp = step_->getNrInputs();
    if ( nrinp == 0 )
	return;

    uiParent* prnt = grp ? (uiParent*)grp : (uiParent*)this;
    const int nrrows = nrinp==-1 ? 2 : nrinp;
    uiTable::Setup ts( nrrows, 1 );
    multiinpfld_ = new uiTable( prnt, ts, "Step inputs" );
    multiinpfld_->setColumnLabel( 0, uiStrings::sInput() );
    initInputTable( nrinp );
}


void uiStepDialog::setInputsFromWeb()
{
    const Chain::Web& web = step_->getChain().getWeb();
    TypeSet<Chain::Connection> connections;
    web.getConnections( step_->getID(), true, connections );
    for ( int idx=0; idx<step_->getNrInputs(); idx++ )
    {
	Step::InputSlotID inpslotid = step_->getInputSlotID( idx );
	Step::ID outputstepid = Step::cUndefID();
	for ( int cidx=0; cidx<connections.size(); cidx++ )
	{
	    if ( connections[cidx].inputslotid_ == inpslotid )
		outputstepid = connections[cidx].outputstepid_;
	}

	if ( outputstepid == Step::cUndefID() )
	    continue;

	Step* inputstep = step_->getChain().getStepFromID( outputstepid );
	mDynamicCastGet(uiComboBox*,cb,
		multiinpfld_->getCellObject(RowCol(idx,0)));
	if ( inputstep && cb ) cb->setCurrentItem( inputstep->userName() );
    }
}


void uiStepDialog::initInputTable( int nr )
{
    if ( !multiinpfld_ ) return;

    BufferStringSet stepnames;
    getStepNames( stepnames );
    multiinpfld_->clearTable();
    for ( int idx=0; idx<nr; idx++ )
    {
	uiComboBox* cb = new uiComboBox( 0, "Steps" );
	cb->addItems( stepnames );
	multiinpfld_->setCellObject( RowCol(idx,0), cb );
    }
}


void uiStepDialog::getStepNames( BufferStringSet& names ) const
{
    Chain& chain = step_->getChain();
    for ( int idx=0; idx<chain.nrSteps(); idx++ )
	if ( step_ != chain.getStep(idx) )
	    names.add( chain.getStep(idx)->userName() );
}


void uiStepDialog::addNameFld( uiObject* alignobj, bool leftalign )
{
    uiSeparator* sep = 0;
    if ( alignobj )
    {
	sep = new uiSeparator( this, "namefld sep" );
	sep->attach( stretchedBelow, alignobj );
    }

    namefld_ = new uiGenInput( this, tr("Name for this step"),
			       step_->userName() );
    namefld_->setElemSzPol( uiObject::Wide );
    if ( alignobj )
    {
	namefld_->attach( ensureBelow, sep );
	namefld_->attach(
	    leftalign ? leftAlignedBelow : alignedBelow, alignobj );
    }
}


void uiStepDialog::addNameFld( uiGroup* aligngrp, bool leftalign )
{
    uiObject* alignobj = aligngrp ? aligngrp->attachObj() : 0;
    addNameFld( alignobj, leftalign );
}


void uiStepDialog::addConnectionFromMultiInput()
{
    for ( int idx=0; idx<step_->getNrInputs(); idx++ )
    {
	mDynamicCastGet(uiComboBox*,cb,
		multiinpfld_->getCellObject(RowCol(idx,0)));
	const char* stepnm = cb ? cb->text() : 0;
	if ( !stepnm ) continue;

	Step* outstep = step_->getChain().getStepFromName( stepnm );
	if ( !outstep ) continue;

	Chain::Connection connection( outstep->getID(), 0,
				step_->getID(), step_->getInputSlotID(idx) );
	step_->getChain().addConnection( connection );
    }
}


void uiStepDialog::addDefaultConnection()
{
    const ObjectSet<Step>& steps = step_->getChain().getSteps();
    const int curidx = steps.indexOf( step_ );
    const Step* prevstep = curidx > 0 ? steps[curidx-1] : 0;
    if ( !prevstep ) return;

    Chain::Connection connection( prevstep->getID(), 0,
	step_->getID(), step_->getInputSlotID(0) );
    step_->getChain().addConnection( connection );
}


bool uiStepDialog::acceptOK( CallBacker* )
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
    {
	uiMSG().error( tr("Please enter a name for this step") );
	return false;
    }

    step_->setUserName( nm.buf() );
    if ( multiinpfld_ )
	addConnectionFromMultiInput();
    else
	addDefaultConnection();

    return true;
}


bool getNamesFromFactory( uiStringSet& uinms, BufferStringSet& nms, bool is2d )
{
    for ( int idx=0; idx<uiStepDialog::factory().size(); idx++ )
    {
	PtrMan<Step> step =
	    Step::factory().create(uiStepDialog::factory().getNames().get(idx));
	if ( !step || (is2d && !step->canHandle2D()) )
	    continue;

	uinms.add( uiStepDialog::factory().getUserNames()[idx] );
	nms.add( uiStepDialog::factory().getNames().get(idx) );
    }

    int* idxs = uinms.getSortIndexes( true, true );
    uinms.useIndexes( idxs );
    nms.useIndexes( idxs );
    delete [] idxs;
    return nms.size();
}

// uiChain


#define mGetIOObjContext is2d_ ? VolProcessing2DTranslatorGroup::ioContext() \
				: VolProcessingTranslatorGroup::ioContext();

uiChain::uiChain( uiParent* p, Chain& chn, bool withprocessnow, bool is2d )
    : uiDialog( p, uiDialog::Setup(tr("Volume Builder: Setup"),
				   mNoDlgTitle, mODHelpKey(mChainHelpID) )
	    .modal(!withprocessnow) )
    , chain_(chn)
    , is2d_(is2d)
{
    chain_.ref();

    tb_ = new uiToolBar( this, tr("Load/Save toolbar"), uiToolBar::Right);
    tb_->addButton( "open", tr("Read stored setup"),mCB(this,uiChain,readPush));
    savebuttonid_ =
	  tb_->addButton( "save", tr("Save setup"),mCB(this,uiChain,savePush) );
    tb_->addButton( "saveas", tr("Save setup as"),mCB(this,uiChain,saveAsPush));

    uiGroup* flowgrp = new uiGroup( this, "Flow group" );

    uiStringSet uinames;
    getNamesFromFactory( uinames, factorysteptypes_, is2d_ );
    const CallBack addcb( mCB(this,uiChain,addStepPush) );
    uiLabel* availablelabel = new uiLabel( flowgrp, tr("Available steps") );
    factorylist_ = new uiListBox( flowgrp, "Processing methods" );
    factorylist_->addItems( uinames );
    factorylist_->resizeHeightToContents();
    factorylist_->setHSzPol( uiObject::Wide );
    factorylist_->selectionChanged.notify( mCB(this,uiChain,factoryClickCB) );
    factorylist_->attach( ensureBelow, availablelabel );
    factorylist_->doubleClicked.notify( addcb );

    const int maxvsz = 15;
    const int nrsteps = uinames.size();
    const int vsz = mMIN( nrsteps, maxvsz );
    factorylist_->box()->setPrefHeightInChar( vsz );

    addstepbutton_ = new uiToolButton( flowgrp, uiToolButton::RightArrow,
					tr("Add step"), addcb );
    addstepbutton_->attach( centeredRightOf, factorylist_ );

    steplist_ = new uiListBox( flowgrp );
    steplist_->setHSzPol( uiObject::Wide );
    steplist_->attach( rightTo, factorylist_ );
    steplist_->attach( ensureRightOf, addstepbutton_ );
    steplist_->selectionChanged.notify( mCB(this,uiChain,stepClickCB) );
    steplist_->doubleClicked.notify( mCB(this,uiChain,stepDoubleClickCB) );

    uiLabel* label = new uiLabel( flowgrp, tr("Used steps") );
    label->attach( alignedAbove, steplist_ );
    label->attach( rightTo, availablelabel );

    moveupbutton_ = new uiToolButton( flowgrp, uiToolButton::UpArrow,
				tr("Move step up"), mCB(this,uiChain,moveUpCB));
    moveupbutton_->attach( rightOf, steplist_ );

    movedownbutton_ = new uiToolButton( flowgrp, uiToolButton::DownArrow,
			    tr("Move step down"), mCB(this,uiChain,moveDownCB));
    movedownbutton_->attach( alignedBelow, moveupbutton_ );

    propertiesbutton_ = new uiToolButton( flowgrp, "settings",
					  tr("Edit this step"),
					  mCB(this,uiChain,propertiesCB) );
    propertiesbutton_->setName( "Settings" );
    propertiesbutton_->attach( alignedBelow, movedownbutton_ );

    removestepbutton_ = new uiToolButton( flowgrp, "remove",
		tr("Remove step from flow"), mCB(this,uiChain,removeStepPush) );
    removestepbutton_->attach( alignedBelow, propertiesbutton_ );

    flowgrp->setHAlignObj( steplist_ );

    IOObjContext ctxt = mGetIOObjContext;
    ctxt.forread_ = false;

    objfld_ = new uiIOObjSel( this, ctxt, tr("On OK, store As") );
    objfld_->setInput( chain_.storageID() );
    objfld_->setConfirmOverwrite( false );
    objfld_->attach( alignedBelow, flowgrp );

    if ( withprocessnow )
    {
	enableSaveButton( tr("Process on OK") );
	bool enabled = false;
	Settings::common().getYN( sKeySettingKey(), enabled );
	setSaveButtonChecked( enabled );
    }

    updateList();
    updateButtons();
}


uiChain::~uiChain()
{
    chain_.unRef();
}


void uiChain::setChain( Chain& chn )
{
    IOPar par;
    chn.fillPar( par );
    chain_.usePar( par );

    updateList();
    updateButtons();
}


void uiChain::updObj( const IOObj& ioobj )
{
    chain_.setStorageID( ioobj.key() );
    objfld_->setInput( ioobj.key() );
    updWriteStatus( &ioobj );
}


void uiChain::updWriteStatus( const IOObj* ioobj )
{
    const bool readonlyvbs = !ioobj || ioobj->implReadOnly();
    if ( readonlyvbs )
    {
	setCaption( uiStrings::phrJoinStrings( setup().wintitle_,
					       tr("[Read-only]") ) );
	objfld_->setEmpty();
    }
    else
    {
	setCaption( setup().wintitle_ );
    }

    tb_->setSensitive( savebuttonid_, !readonlyvbs );
}


const MultiID& uiChain::storageID() const
{
    return chain_.storageID();
}


bool uiChain::acceptOK( CallBacker* )
{
    const int nrsteps = chain_.nrSteps();
    if ( nrsteps>0 && chain_.getStep(0) &&
	 chain_.getStep(0)->needsInput() )
    {
	if ( !uiMSG().askGoOn(tr("The first step in the chain needs an input, "
                  "and can thus not be first. Proceed anyway?"), true ) )
	    return false;
    }

    const Step* laststep = chain_.getStep( nrsteps-1 );
    if ( !laststep )
	return false;

    chain_.setOutputSlot( laststep->getID(), laststep->getOutputSlotID(0) );

    if ( !doSave() )
	return false;

    if ( hasSaveButton() )
    {
	Settings::common().setYN( sKeySettingKey(), saveButtonChecked() );
	Settings::common().write();
    }

    return true;
}


bool uiChain::doSave()
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( !ioobj )
	return doSaveAs();

    uiString errmsg;
    if ( !VolProcessingTranslator::store(chain_,ioobj,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    chain_.setStorageID( ioobj->key() );
    updWriteStatus( ioobj );

    return true;
}


bool uiChain::doSaveAs()
{
    IOObjContext ctxt = mGetIOObjContext;
    ctxt.forread_ = false;
    uiIOObjSelDlg dlg( this, ctxt, tr("Volume Builder Setup") );
    if ( !dlg.go() || !dlg.nrChosen() )
	 return false;

    uiString errmsg;
    const IOObj* ioobj = dlg.ioObj();
    if ( !VolProcessingTranslator::store(chain_,ioobj,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    updObj( *ioobj );
    return true;
}


void uiChain::updateList()
{
    NotifyStopper stopper( steplist_->selectionChanged );
    int idx=0;
    for ( ; idx<chain_.nrSteps(); idx ++ )
    {
	const char* key = chain_.getStep(idx)->factoryKeyword();
	const char* username = chain_.getStep(idx)->userName();
	uiString displayname = toUiString(username);
	if ( !displayname )
	{
	    displayname = chain_.getStep(idx)->factoryDisplayName();
	    if ( !displayname )
		displayname = toUiString(key);
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
    const bool factoryselected = factorylist_->firstChosen()!=-1;
    const int stepsel = steplist_->firstChosen();

    addstepbutton_->setSensitive( factoryselected );
    removestepbutton_->setSensitive( stepsel!=-1 );

    moveupbutton_->setSensitive( stepsel>0 );
    movedownbutton_->setSensitive( stepsel!=-1 &&
	    stepsel!=steplist_->size()-1 );

    const bool hasdlg = stepsel>=0 &&
	factorysteptypes_.isPresent( chain_.getStep(stepsel)->factoryKeyword());

    propertiesbutton_->setSensitive( hasdlg );
}


bool uiChain::showPropDialog( int idx )
{
    Step* step = chain_.getStep( idx );
    if ( !step ) return false;

    PtrMan<uiStepDialog> dlg = uiStepDialog::factory().create(
				step->factoryKeyword(), this, step, is2d_ );
    if ( !dlg )
    {
	uiMSG().error( tr("Internal error. Step cannot be created") );
	return false;
    }

    bool ret = dlg->go();
    if ( ret )
	updateList();

    return ret;
}


void uiChain::readPush( CallBacker* )
{
    IOObjContext ctxt = mGetIOObjContext;
    ctxt.forread_ = true;
    uiIOObjSelDlg dlg( this, ctxt );
    dlg.selGrp()->setConfirmOverwrite( false );
    if ( !dlg.go() || !dlg.nrChosen() )
    {
	const IOObj* ioobj = objfld_->ioobj(true);
	if ( ioobj && !ioobj->implExists(false) )
	    updWriteStatus(0);

	return;
    }

    uiString errmsg;
    const IOObj* ioobj = dlg.ioObj();
    if ( !VolProcessingTranslator::retrieve(chain_,ioobj,errmsg) )
    {
	updateList();
	uiMSG().error( errmsg );
	return;
    }

    updObj( *ioobj );
    updateList();
}


void uiChain::saveAsPush( CallBacker* )
{ doSaveAs(); }


void uiChain::savePush( CallBacker* )
{ doSave(); }


void uiChain::factoryClickCB(CallBacker*)
{
    if ( factorylist_->firstChosen() == -1 )
	return;

    steplist_->chooseAll(false);
    updateButtons();
}


void uiChain::stepClickCB(CallBacker*)
{
    if ( steplist_->firstChosen() == -1 )
	return;

    factorylist_->chooseAll(false);
    updateButtons();
}


void uiChain::stepDoubleClickCB(CallBacker*)
{
    factorylist_->chooseAll(false);
    updateButtons();

    showPropDialog( steplist_->currentItem() );
}


void uiChain::addStepPush(CallBacker*)
{
    const int sel = factorylist_->firstChosen();
    if ( sel == -1 )
	return;

    addStep( factorysteptypes_.get(sel) );
}


void uiChain::addStep( const char* steptype )
{
    Step* step = Step::factory().create( steptype );
    if ( !step ) return;

    if ( chain_.nrSteps()==0 && step->getNrInputs()>0 && step->needsInput() )
    {
	uiMSG().error(
	    tr("The %1 cannot be used as an initial volume. "
		"Please select one of the following as initial step:\n%2.")
		.arg( step->factoryDisplayName() )
		.arg( getPossibleInitialStepNames(is2d_) ) );

	delete step;
	return;
    }

    chain_.addStep( step );
    updateList();
    steplist_->chooseAll( false );
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


void uiChain::emptyChain()
{
    if ( chain_.nrSteps()<1 )
	return;

    if ( !uiMSG().askGoOn( tr("You have existing setup steps in your volume "
		    "builder, do you want to remove them?") ) )
	return;

    int curitm = steplist_->size()-1;
    while ( curitm>=0 )
    {
	steplist_->removeItem(curitm);
	chain_.removeStep( curitm );
	curitm--;
    }

    updateButtons();
}


void uiChain::removeStepPush( CallBacker* )
{
    int curitm = steplist_->firstChosen();
    if ( curitm < 0 )
	return;

    chain_.removeStep( curitm );
    updateList();
    steplist_->chooseAll(false);
    if ( curitm >= steplist_->size() )
	curitm--;
    if ( curitm >= 0 )
	steplist_->setCurrentItem( curitm );

    updateButtons();
}


void uiChain::moveUpCB( CallBacker* )
{
    const int idx = steplist_->firstChosen();

    if ( idx<1 ) return;

    chain_.swapSteps( idx, idx-1 );
    updateList();

    steplist_->setChosen( idx-1, true );
    updateButtons();
}


void uiChain::moveDownCB( CallBacker* )
{
    const int idx = steplist_->firstChosen();
    if ( idx<0 || idx>=chain_.nrSteps() )
	return;

    chain_.swapSteps( idx, idx+1 );
    updateList();
    steplist_->setChosen( idx+1, true );
    updateButtons();
}


void uiChain::propertiesCB( CallBacker* )
{
    const int idx = steplist_->firstChosen();
    if ( idx<0 ) return;

    showPropDialog( idx );
}


uiString uiChain::getPossibleInitialStepNames( bool is2d )
{
    mDefineStaticLocalObject( uiString, names, (uiString::emptyString()) );

    if ( names.isEmpty() )
    {
	uiStringSet possiblenames;
	for ( int idx=0; idx<uiStepDialog::factory().getNames().size(); idx++ )
	{
	    const char* steptype =
		uiStepDialog::factory().getNames()[idx]->buf();

	    PtrMan<Step> step = Step::factory().create( steptype );
	    if ( !step ) continue;

	    if ( is2d && !step->canHandle2D() )
		continue;

	    if ( step->getNrInputs()>0 && step->needsInput() )
		continue;

	    possiblenames += uiStepDialog::factory().getUserNames()[idx];
	}

	names = possiblenames.createOptionString( false, -1, '\n' );
    }

    return names;
}

} // namespace VolProc
