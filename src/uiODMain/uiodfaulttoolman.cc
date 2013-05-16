/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiodfaulttoolman.h"

#include "emeditor.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "keyboardevent.h"
#include "randcolor.h"
#include "thread.h"
#include "timefun.h"
#include "undo.h"
#include "visdataman.h"
#include "visfaultsticksetdisplay.h"
#include "visfaultdisplay.h"
#include "visselman.h"

#include "uitoolbutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "pixmap.h"


uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiODMain& appl,
						  const Setup& su )
    : uiDialog( &appl, uiDialog::Setup("Faultstick transfer",
			"Transfer settings","104.1.8").modal(false) )
    , displayifnot_( su.displayifnot_ )
    , saveifdisplayed_( su.saveifdisplayed_ )
    , colormodechg( this )
{
    setCtrlStyle( LeaveOnly );

    uiLabel* colormodelbl = new uiLabel( this, "Output color selection:" );

    BufferStringSet serialbss; serialbss.add( "Inherit" );
    serialbss.add( "Random" ); serialbss.add( "User-defined" );
    serialcolormodefld_ = new uiGenInput(this,"",StringListInpSpec(serialbss));
    serialcolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    serialcolormodefld_->attach( alignedBelow, colormodelbl );

    BufferStringSet existsbss;
    existsbss.add( "Current" ); existsbss.add( "User-defined" );
    existscolormodefld_ = new uiGenInput(this,"",StringListInpSpec(existsbss));
    existscolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    existscolormodefld_->attach( alignedBelow, colormodelbl );

    BufferStringSet singlebss; singlebss.add( "User-defined" );
    singlecolormodefld_ = new uiGenInput(this,"",StringListInpSpec(singlebss));
    singlecolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    singlecolormodefld_->attach( alignedBelow, colormodelbl );

    setColorMode( su.colormode_ );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, serialcolormodefld_ );

    uiLabel* aftercopymovelbl = new uiLabel( this, "After copy / move:" );
    aftercopymovelbl->setMinimumWidth( 250 );
    aftercopymovelbl->attach( alignedBelow, horsep );

    displayfld_ = new uiCheckBox( this, "Display" );
    displayfld_->setChecked( displayifnot_ );
    displayfld_->attach( alignedBelow, aftercopymovelbl );
    displayfld_->activated.notify(mCB(this,uiFaultStickTransferDlg,displayCB));

    savefld_ = new uiCheckBox( this, "Save" );
    savefld_->setChecked( saveifdisplayed_ );
    savefld_->attach( rightOf, displayfld_ );
    savefld_->activated.notify( mCB(this,uiFaultStickTransferDlg,saveCB) );
}



void uiFaultStickTransferDlg::displayCB( CallBacker* )
{
    displayifnot_ = displayfld_->isChecked();
    setOutputDisplayed( !displayfld_->sensitive() );
}


void uiFaultStickTransferDlg::saveCB( CallBacker* )
{ saveifdisplayed_ = savefld_->isChecked(); }


void uiFaultStickTransferDlg::setOutputDisplayed( bool yn )
{
    NotifyStopper displaynotifystopper( displayfld_->activated );
    NotifyStopper savenotifystopper( savefld_->activated );

    displayfld_->setSensitive( !yn );
    displayfld_->setChecked( yn || displayifnot_ );
    savefld_->setSensitive( displayfld_->isChecked() );
    savefld_->setChecked( !displayfld_->isChecked() || saveifdisplayed_ );
}


void uiFaultStickTransferDlg::setColorMode( int mode )
{
    serialcolormodefld_->attachObj()->display( false );
    existscolormodefld_->attachObj()->display( false );
    singlecolormodefld_->attachObj()->display( false );

    if ( mode <= SerialUserDef )
    {
	serialcolormodefld_->setValue( mode );
	serialcolormodefld_->attachObj()->display( true );
    }
    else if ( mode <= ExistsUserDef )
    {
	existscolormodefld_->setValue( mode - SerialUserDef - 1 );
	existscolormodefld_->attachObj()->display( true );
    }
    else
    {
	singlecolormodefld_->setValue( mode - ExistsUserDef - 1 );
	singlecolormodefld_->attachObj()->display( true );
    }
}


bool uiFaultStickTransferDlg::displayAfterwards() const
{ return displayfld_->isChecked(); }


bool uiFaultStickTransferDlg::saveAfterwards() const
{ return savefld_->isChecked(); }


int uiFaultStickTransferDlg::colorMode() const
{
    if ( serialcolormodefld_->attachObj()->isDisplayed() )
	return serialcolormodefld_->getIntValue();

    if ( existscolormodefld_->attachObj()->isDisplayed() )
	return existscolormodefld_->getIntValue() + SerialUserDef + 1;

    return singlecolormodefld_->getIntValue() + ExistsUserDef + 1;
}


void uiFaultStickTransferDlg::colorModeChg( CallBacker* )
{ colormodechg.trigger(); }


//============================================================================


class FaultStickTransferUndoEvent : public UndoEvent
{
public:
    FaultStickTransferUndoEvent( bool copy, bool saved )
	: copy_( copy )
	, saved_( saved )
    {}

    const char* getStandardDesc() const
    { return copy_ ? "Copy sticks" : "Move sticks"; }

    bool unDo()
    {
	if ( saved_ )
	    uiMSG().message( "Will not undo saving the output file!" );
	return true;
    }

    bool reDo()
    { return true; }

protected:
    bool copy_;
    bool saved_;
};


//============================================================================

#define mDefineKey( key, str ) \
const char* uiODFaultToolMan::key()	{ return str; }

mDefineKey( sKeyCopySelection,     "Copy selection to" );
mDefineKey( sKeyMoveSelection,     "Move selection to" );
mDefineKey( sKeyToFault,           "Fault" );
mDefineKey( sKeyToFaultStickSet,   "FaultStickSet" );
mDefineKey( sKeyCreateSingleNew,   "Create single new" );
mDefineKey( sKeyCreateNewInSeries, "Create new in series" );
mDefineKey( sKeyMergeWithExisting, "Merge with existing" );
mDefineKey( sKeyReplaceExisting,   "Replace existing" );

#define mCurItem( combo, key ) ( !strcmp(combo->text(), key()) )


uiODFaultToolMan::uiODFaultToolMan( uiODMain& appl )
    : appl_( appl )
    , tracktbwashidden_( false )
    , selectmode_( false )
    , settingsdlg_( 0 )
    , colorbutcolor_( Color(0,0,0) )
    , usercolor_( Color(0,0,255) )
    , randomcolor_ ( getRandStdDrawColor() )
    , flashcolor_( Color(0,0,0) )
{
    toolbar_ = new uiToolBar( &appl_, "Fault stick control", uiToolBar::Bottom);
    editbutidx_ = toolbar_->addButton( "editsticks", "Edit sticks",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    selbutidx_ = toolbar_->addButton( "selectsticks", "Select sticks",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    toolbar_->addSeparator();

    transfercombo_ = new uiComboBox( toolbar_, "Stick transfer action" );
    transfercombo_->setToolTip( transfercombo_->name() );
    transfercombo_->addItem( sKeyCopySelection() );
    transfercombo_->addItem( sKeyMoveSelection() );
    toolbar_->addObject( transfercombo_ );

    outputtypecombo_ = new uiComboBox( toolbar_, "Output type" );
    outputtypecombo_->setToolTip( outputtypecombo_->name() );
    outputtypecombo_->addItem( sKeyToFault() );
    outputtypecombo_->addItem( sKeyToFaultStickSet() );
    outputtypecombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputTypeChg) );
    toolbar_->addObject( outputtypecombo_ );

    outputactcombo_ = new uiComboBox( toolbar_, "Output operation" );
    outputactcombo_->setToolTip( outputactcombo_->name() );
    outputactcombo_->addItem( sKeyCreateSingleNew() );
    outputactcombo_->addItem( sKeyCreateNewInSeries() );
    outputactcombo_->addSeparator();
    outputactcombo_->addItem( sKeyMergeWithExisting() );
    outputactcombo_->addItem( sKeyReplaceExisting() );
    outputactcombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputActionChg) );
    toolbar_->addObject( outputactcombo_ );

    outputnamecombo_ = new uiComboBox( toolbar_, "Output name" );
    outputnamecombo_->setToolTip( outputnamecombo_->name() );
    outputnamecombo_->setReadOnly( false );
    outputnamecombo_->setMinimumWidth( 150 );
    outputnamecombo_->editTextChanged.notify(
				mCB(this,uiODFaultToolMan,outputEditTextChg) );
    outputnamecombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputComboSelChg) );
    outputnamecombo_->addItem( "" );
    toolbar_->addObject( outputnamecombo_ );

    auxfaultwrite_ = new uiSurfaceWrite( &appl_,
	    uiSurfaceWrite::Setup(EMFault3DTranslatorGroup::keyword()));
    auxfaultwrite_->getObjSel()->setConfirmOverwrite( false );
    auxfaultwrite_->getObjSel()->selectionDone.notify(
				mCB(this,uiODFaultToolMan,outputSelectedCB) );
    auxfsswrite_ = new uiSurfaceWrite( &appl_,
	    uiSurfaceWrite::Setup(EMFaultStickSetTranslatorGroup::keyword()));
    auxfsswrite_->getObjSel()->setConfirmOverwrite( false );
    auxfsswrite_->getObjSel()->selectionDone.notify(
				mCB(this,uiODFaultToolMan,outputSelectedCB) );

    outputselbut_ = new uiPushButton( toolbar_, "Select",
				mCB(this,uiODFaultToolMan,selectOutputCB),
				false );
    outputselbut_->setToolTip( "Select output" );
    toolbar_->addObject( outputselbut_ );

    colorbut_ = new uiToolButton( toolbar_, uiIcon::None(), "Output color",
				mCB(this,uiODFaultToolMan,colorPressedCB) );
    colorbut_->setToolTip( colorbut_->name() );
    toolbar_->addObject( colorbut_ );

    settingsbutidx_ = toolbar_->addButton( "tools", "More transfer settings",
	    			mCB(this,uiODFaultToolMan,settingsToggleCB),
				true );

    gobutidx_ = toolbar_->addButton( "gobutton", "Transfer selected sticks",
	    			mCB(this,uiODFaultToolMan,transferSticksCB),
				false );

    toolbar_->addSeparator();

    removalbutidx_ = toolbar_->addButton( "removesticks",
	    			"Remove selected sticks",
	    			mCB(this,uiODFaultToolMan,stickRemovalCB),
				false );
    toolbar_->addSeparator();


    undobutidx_ = toolbar_->addButton( "undo", "Undo",
	    			mCB(this,uiODFaultToolMan,undoCB), false );

    redobutidx_ = toolbar_->addButton( "redo", "Redo",
	    			mCB(this,uiODFaultToolMan,redoCB), false );

    toolbar_->addSeparator();

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    appl_.applMgr().visServer()->objectaddedremoved.notify(
				mCB(this,uiODFaultToolMan,addRemoveVisObjCB) );

    appl_.postFinalise().notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.notify( mCB(this,uiODFaultToolMan,deselTimerCB) );
    editreadytimer_.tick.notify( mCB(this,uiODFaultToolMan,editReadyTimerCB) );
    flashtimer_.tick.notify( mCB(this,uiODFaultToolMan,flashOutputTimerCB) );
    EM::EMM().undo().undoredochange.notify(
				mCB(this,uiODFaultToolMan,updateToolbarCB) );
    uiMain::keyboardEventHandler().keyPressed.notify(
				mCB(this,uiODFaultToolMan,keyPressedCB) );
    uiMain::keyboardEventHandler().keyReleased.notify(
				mCB(this,uiODFaultToolMan,keyReleasedCB) );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    visBase::DM().selMan().selnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.remove( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    appl_.applMgr().visServer()->objectaddedremoved.remove(
				mCB(this,uiODFaultToolMan,addRemoveVisObjCB) );

    appl_.postFinalise().remove( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    IOM().surveyChanged.remove( mCB(this,uiODFaultToolMan,surveyChg) );
    EM::EMM().undo().undoredochange.remove(
				mCB(this,uiODFaultToolMan,updateToolbarCB) );

    delete toolbar_;

    if ( settingsdlg_ )
	delete settingsdlg_;

    uiMain::keyboardEventHandler().keyPressed.remove(
				mCB(this,uiODFaultToolMan,keyPressedCB) );
    uiMain::keyboardEventHandler().keyReleased.remove(
				mCB(this,uiODFaultToolMan,keyReleasedCB) );
}


void uiODFaultToolMan::finaliseDoneCB( CallBacker* )
{
    // Hide auxiliary uiSurfaceWrites, because they seem to need a parent
    // to have their surface list dialog popped up modal.
    auxfaultwrite_->display( false );
    auxfsswrite_->display( false );

    clearCurDisplayObj();
}


uiToolBar* uiODFaultToolMan::getToolBar()
{ return toolbar_; }


void uiODFaultToolMan::displayModeChg( CallBacker* )
{ editSelectToggleCB( 0 ); }


void uiODFaultToolMan::treeItemSelCB( CallBacker* cber )
{
    deseltimer_.stop();
    mCBCapsuleUnpack( int, selid, cber );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCast( visSurvey::FaultStickSetDisplay*, curfssd_, dataobj );
    mDynamicCast( visSurvey::FaultDisplay*, curfltd_, dataobj );

    if ( curfssd_ || curfltd_ )
    {
	curemid_ = curfssd_ ? curfssd_->getEMID() : curfltd_->getEMID();

	const EM::EMObject* emobj = EM::EMM().getObject( curemid_ );
	if ( !emobj || emobj->isEmpty() )
	{
	    toolbar_->turnOn( selbutidx_, false );
	    selectmode_ = false;
	}

	editSelectToggleCB( 0 );
	processOutputName();
	enableToolbar( true );

	IOM().surveyChanged.notifyIfNotNotified(
				    mCB(this,uiODFaultToolMan,surveyChg) );

	CallBack cb = mCB(this,uiODFaultToolMan,displayModeChg);
	if ( curfssd_ )
	    curfssd_->displaymodechange.notify( cb );
	if ( curfltd_ )
	    curfltd_->displaymodechange.notify( cb );
    }
    else
	clearCurDisplayObj();
}


void uiODFaultToolMan::treeItemDeselCB( CallBacker* cber )
{
    mCBCapsuleUnpack( int, selid, cber );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCastGet( visSurvey::FaultStickSetDisplay*, oldfssd, dataobj );
    mDynamicCastGet( visSurvey::FaultDisplay*, oldfltd, dataobj );
    if ( oldfssd==curfssd_ && oldfltd==curfltd_ )
    {
	CallBack cb = mCB(this,uiODFaultToolMan,displayModeChg);
	if ( curfssd_ )
	    curfssd_->displaymodechange.remove( cb );
	if ( curfltd_ )
	    curfltd_->displaymodechange.remove( cb );

	deseltimer_.start( 100, true );
    }
}


void uiODFaultToolMan::addRemoveEMObjCB( CallBacker* cb )
{
    if ( curemid_ == -1 )
	return;

    if ( !EM::EMM().getObject(curemid_) )
	deseltimer_.start( 100, true );
}


struct DisplayCacheObj
{
    MultiID mid_;
    int sceneid_;
    bool isdisplayed_;
};

static ObjectSet<DisplayCacheObj> displaycache_;


void uiODFaultToolMan::addRemoveVisObjCB( CallBacker* cb )
{
    deepErase( displaycache_ );

    if ( settingsdlg_ )
	settingsdlg_->setOutputDisplayed( isOutputDisplayed() );
}


void uiODFaultToolMan::deselTimerCB( CallBacker* )
{ clearCurDisplayObj(); }


void uiODFaultToolMan::clearCurDisplayObj()
{
    curfssd_ = 0;
    curfltd_ = 0;
    curemid_ = -1;
    enableToolbar( false );
}


#define mOutputNameComboSetTextColorSensitivityHack( color ) \
    outputnamecombo_->setTextColor( toolbar_->isSensitive() && \
	    outputnamecombo_->sensitive() ? color : Color(128,128,128) );

void uiODFaultToolMan::enableToolbar( bool yn )
{
    if ( yn == toolbar_->isSensitive() )
	return;

    uiToolBar* tracktb_ = appl_.applMgr().visServer()->getTrackTB();
    if ( yn )
    {
	tracktbwashidden_ = tracktb_->isHidden();
	tracktb_->display( false );

	const bool selecting = toolbar_->isOn( selbutidx_ );
	showSettings( selecting && toolbar_->isOn(settingsbutidx_) );
    }

    toolbar_->display( yn );
    toolbar_->setSensitive( yn );

    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );

    if ( !yn )
    {
	showSettings( false );
	appl_.applMgr().visServer()->turnSelectionModeOn( false );

	if ( tracktb_->isHidden() )
	    tracktb_->display( !tracktbwashidden_ );
    }
}


void uiODFaultToolMan::showSettings( bool yn )
{
    if ( !settingsdlg_ )
    {

	if ( yn )
	{
	    settingsdlg_ = new uiFaultStickTransferDlg( appl_, settingssetup_ );
	    settingsdlg_->colormodechg.notify(
				mCB(this,uiODFaultToolMan,colorModeChg) );
	    settingsdlg_->windowClosed.notify(
				mCB(this,uiODFaultToolMan,settingsClosedCB) );
	    settingsdlg_->go();
	    settingsdlg_->setOutputDisplayed( isOutputDisplayed() );
	}

	return;
    }

    if ( yn != settingsdlg_->isHidden() )
	return;

    if ( yn )
	settingsdlg_->show();
    else
    {
	NotifyStopper ns( settingsdlg_->windowClosed );
	settingsdlg_->close();

	// Remember last position
	uiRect frame = settingsdlg_->geometry();
	settingsdlg_->setCornerPos( frame.get(uiRect::Left),
				    frame.get(uiRect::Top) );
    }
}


bool uiODFaultToolMan::areSticksAccessible() const
{ return curfssd_ || ( curfltd_ && curfltd_->areSticksDisplayed() ); }


void uiODFaultToolMan::enableStickAccess( bool yn )
{
    if ( curfltd_ && curfltd_->areSticksDisplayed()!=yn )
	curfltd_->display( yn, !yn || curfltd_->arePanelsDisplayed() );
}


void uiODFaultToolMan::editSelectToggleCB( CallBacker* cb )
{
    if ( toolbar_->isOn(editbutidx_) && toolbar_->isOn(selbutidx_) )
    {
	selectmode_ = !selectmode_;
	toolbar_->turnOn( selectmode_ ? editbutidx_ : selbutidx_, false );
    }
    else if ( !toolbar_->isOn(editbutidx_) && !toolbar_->isOn(selbutidx_) )
    {
	if ( cb )
	    enableStickAccess( false );

	if ( areSticksAccessible() )
	    toolbar_->turnOn( selectmode_ ? selbutidx_ : editbutidx_, true );
    }
    else
    {
	if ( cb )
	    enableStickAccess( true );

	if ( areSticksAccessible() )
	    selectmode_ = toolbar_->isOn( selbutidx_ );
	else
	    toolbar_->turnOn( selectmode_ ? selbutidx_ : editbutidx_, false );
    }

    if ( curfssd_ )
	curfssd_->setStickSelectMode( selectmode_ );
    if ( curfltd_ )
	curfltd_->setStickSelectMode( selectmode_ );

    updateToolbarCB( 0 );

    const bool selecting = toolbar_->isOn( selbutidx_ );
    showSettings( selecting && toolbar_->isOn(settingsbutidx_) );
    appl_.applMgr().visServer()->turnSelectionModeOn( selecting );
}


void uiODFaultToolMan::updateToolbarCB( CallBacker* )
{
    const bool selecting = toolbar_->isOn( selbutidx_ );

    toolbar_->setSensitive( settingsbutidx_, selecting );
    toolbar_->setSensitive( removalbutidx_, selecting );
    transfercombo_->setSensitive( selecting );
    outputtypecombo_->setSensitive( selecting );
    outputactcombo_->setSensitive( selecting );
    outputnamecombo_->setSensitive( selecting );
    outputselbut_->setSensitive( selecting );
    colorbut_->setSensitive( selecting );

    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );

    toolbar_->setSensitive( undobutidx_, EM::EMM().undo().canUnDo() );
    BufferString undotooltip( "Undo" );
    if ( EM::EMM().undo().canUnDo() )
    {
	undotooltip += " "; undotooltip += EM::EMM().undo().unDoDesc();
    }
    toolbar_->setToolTip( undobutidx_, undotooltip.buf() );

    toolbar_->setSensitive( redobutidx_, EM::EMM().undo().canReDo() );
    BufferString redotooltip( "Redo" );
    if ( EM::EMM().undo().canReDo() )
    {
	redotooltip += " "; redotooltip += EM::EMM().undo().reDoDesc();
    }
    toolbar_->setToolTip( redobutidx_, redotooltip.buf() );
}


uiIOObjSel* uiODFaultToolMan::getObjSel()
{
    if ( mCurItem(outputtypecombo_, sKeyToFaultStickSet) )
	return auxfsswrite_->getObjSel();

    return auxfaultwrite_->getObjSel();
}


const uiIOObjSel* uiODFaultToolMan::getObjSel() const
{ return const_cast<uiODFaultToolMan*>(this)->getObjSel(); }


void uiODFaultToolMan::outputEditTextChg( CallBacker* )
{
    setAuxSurfaceWrite( outputnamecombo_->text() );
    flashReset();
    editreadytimer_.start( 200, false );
}


void uiODFaultToolMan::editReadyTimerCB( CallBacker* )
{
    static uiPoint lastpos( uiPoint::udf() );

    if ( outputnamecombo_->isCursorInside() ||
	 !lastpos.isDefined() || lastpos==uiCursorManager::cursorPos() )
    {
	lastpos = uiCursorManager::cursorPos();
	return;
    }

    lastpos = uiPoint::udf();
    editreadytimer_.stop();
    processOutputName();
}


BufferStringSet& uiODFaultToolMan::getOutputItems()
{
    const bool tofault = mCurItem(outputtypecombo_, sKeyToFault);

    if ( !isInCreateMode() )
	return tofault ? allfaultitems_ : allfssitems_;

    if ( isInSerialMode() )
	return tofault ? serialfaultitems_ : serialfssitems_;

    return tofault ? singlefaultitems_ : singlefssitems_;
}


static int removeSerialNumber( BufferString& objname )
{
    char* lastptr = objname.buf() + objname.size() - 1;
    char* ptr = lastptr;

    while ( ptr>=objname.buf() && isdigit(*ptr) )
	ptr--;

    if ( ptr<objname.buf() || *ptr!='_' )
	return -1;				// Nothing removed

    *ptr = '\0';
    return ptr==lastptr ? mUdf(int) : toInt(ptr+1);
}


static void addOutputItem( const char* newitem, BufferStringSet& items )
{
    const int maxnritems = 10;

    for ( int idx=items.size()-1; idx>=0; idx-- )
    {
	if ( items.get(idx) == newitem )
	    items.removeSingle( idx );
    }
    items.insertAt( new BufferString(newitem), 0 );

    for ( int idx=items.size()-1; idx>=maxnritems; idx-- )
	items.removeSingle( idx );
}


void uiODFaultToolMan::updateOutputItems( bool clearcuritem )
{
    const bool tofault = mCurItem(outputtypecombo_, sKeyToFault);

    BufferString fullnm = outputnamecombo_->text();
    if ( fullnm.isEmpty() )
	return;

    BufferString basenm = fullnm;
    if ( removeSerialNumber(basenm) == -1 )
	addOutputItem( fullnm, tofault ? singlefaultitems_ : singlefssitems_ );
    else
    {
	basenm += "_";
	addOutputItem( basenm, tofault ? serialfaultitems_ : serialfssitems_ );
    }

    addOutputItem( fullnm, tofault ? allfaultitems_ : allfssitems_ );

    if ( clearcuritem )
	setOutputName( "" );

    publishOutputItems();
}


void uiODFaultToolMan::outputComboSelChg( CallBacker* )
{
    editreadytimer_.stop();
    publishOutputItems();
}


void uiODFaultToolMan::outputActionChg( CallBacker* )
{ publishOutputItems(); }


void uiODFaultToolMan::outputTypeChg( CallBacker* )
{ publishOutputItems(); }


void uiODFaultToolMan::publishOutputItems()
{
    outputnamecombo_->editTextChanged.disable();
    BufferString curtext = outputnamecombo_->text();
    outputnamecombo_->setEmpty();
    outputnamecombo_->addItem( "" );
    outputnamecombo_->addItems( getOutputItems() );
    outputnamecombo_->editTextChanged.enable();

    setOutputName( curtext );
    processOutputName();
}


void uiODFaultToolMan::colorPressedCB( CallBacker* cb )
{
    if ( selectColor(colorbutcolor_) )
	outputColorChg( cb );
}


void uiODFaultToolMan::outputColorChg( CallBacker* cb )
{
    colorbut_->setToolTip( "Output color [user-defined]" );

    if ( cb )
    {
	usercolor_ = colorbutcolor_;
	usercolorlink_ = getObjSel()->inpBox()->text();
	updateColorMode();
    }
    else if ( randomColor() )
    {
	colorbutcolor_ = randomcolor_;
	colorbut_->setToolTip( "Output color [random]" );
    }
    else
    {
	colorbutcolor_ = usercolor_;

	if ( currentColor() || inheritColor() )
	{
	    MultiID mid = auxfaultwrite_->getObjSel()->validKey();
	    if ( !isOutputNameUsed(auxfaultwrite_) )
		mid = auxfsswrite_->getObjSel()->validKey();

	    if ( !mid.isEmpty() )
	    {
		const EM::ObjectID emid  = EM::EMM().getObjectID( mid );
		const EM::EMObject* emobj = EM::EMM().getObject( emid );

		IOPar iopar;
		Color curcolor;
		if ( emobj )
		    curcolor = emobj->preferredColor();
		else
		    EM::EMM().readPars( mid, iopar );

		if ( emobj || iopar.get(sKey::Color(),curcolor) )
		{
		    colorbutcolor_ = curcolor;
		    colorbut_->setToolTip( currentColor() ?
					   "Output color [current]" :
					   "Output color [predecessor]" );
		}
	    }
	}
    }

    ioPixmap colorpm( 20, 20 );
    colorpm.fill( colorbutcolor_ );
    colorbut_->setPixmap( colorpm );
}


void uiODFaultToolMan::colorModeChg( CallBacker* cb )
{
    const bool userdef = !randomColor() && !inheritColor() && !currentColor();

    usercolorlink_ = userdef ? getObjSel()->inpBox()->text() : "";

    if ( randomColor() )
	randomcolor_ = getRandStdDrawColor();

    processOutputName();
}


#define mGetSetting( settingsfunc, setupmember ) \
( settingsdlg_ ? settingsdlg_->settingsfunc : settingssetup_.setupmember )

void uiODFaultToolMan::updateColorMode()
{
    const bool userdef = usercolorlink_ == getObjSel()->inpBox()->text();
    const int curmode = mGetSetting( colorMode(), colormode_ );
    const bool random = curmode == uiFaultStickTransferDlg::Random;

    if ( isInSerialMode() )
    {
	if ( userdef )
	    settingssetup_.colormode_ = uiFaultStickTransferDlg::SerialUserDef;
	else if ( random )
	    settingssetup_.colormode_ = uiFaultStickTransferDlg::Random;
	else
	    settingssetup_.colormode_ = uiFaultStickTransferDlg::Inherit;
    }
    else if ( isOutputNameUsed() )
    {
	if ( userdef )
	    settingssetup_.colormode_ = uiFaultStickTransferDlg::ExistsUserDef;
	else
	    settingssetup_.colormode_ = uiFaultStickTransferDlg::Current;
    }
    else
	settingssetup_.colormode_ = uiFaultStickTransferDlg::SingleUserDef;

    if ( settingsdlg_ )
    {
        NotifyStopper ns( settingsdlg_->colormodechg );
	settingsdlg_->setColorMode( settingssetup_.colormode_ );
        settingsdlg_->setOutputDisplayed( isOutputDisplayed() );
    }
}


void uiODFaultToolMan::selectOutputCB( CallBacker* )
{ getObjSel()->doSel( 0 ); }


void uiODFaultToolMan::outputSelectedCB( CallBacker* )
{
    setOutputName( getObjSel()->inpBox()->text() );
    processOutputName();
}


void uiODFaultToolMan::stickRemovalCB( CallBacker* )
{
    if ( curemid_ < 0 )
	return;

    EM::EMObject* srcemobj = EM::EMM().getObject( curemid_ );
    mDynamicCastGet( EM::Fault*, srcfault, srcemobj );
    if ( !srcfault )
	return;

    if ( !srcfault->geometry().nrSelectedSticks() )
    {
	uiMSG().error( "No selected fault stick(s) to remove" );
	return;
    }

    srcfault->geometry().removeSelectedSticks( true );
    srcfault->setChangedFlag();
    EM::EMM().undo().setUserInteractionEnd( EM::EMM().undo().currentEventID() );
}


void uiODFaultToolMan::transferSticksCB( CallBacker* )
{
    NotifyStopper ns( EM::EMM().undo().changenotifier );
    MouseCursorChanger mcc( MouseCursor::Wait );

    if ( curemid_ < 0 )
	return;

    RefMan<EM::EMObject> srcemobj = EM::EMM().getObject( curemid_ );
    mDynamicCastGet( EM::Fault*, srcfault, srcemobj.ptr() );
    if ( !srcfault )
	return;

    const int oldnrselected = srcfault->geometry().nrSelectedSticks();
    if ( !oldnrselected )
    {
	uiMSG().error( "No selected fault stick(s) to transfer" );
	return;
    }

    const MultiID destmid = getObjSel()->key();
    if ( destmid.isEmpty() )
	return;

    EM::EMM().loadIfNotFullyLoaded( destmid );
    const EM::ObjectID destemid = EM::EMM().getObjectID( destmid );
    RefMan<EM::EMObject> destemobj = EM::EMM().getObject( destemid );
    mDynamicCastGet( EM::Fault*, destfault, destemobj.ptr() );
    if ( !destfault )
	return;

    if ( destfault == srcfault )
    {
	uiMSG().error( "No use to transfer selected sticks to myself" );
	return;
    }

    if ( flashcolor_==Color(255,0,0) &&
	 !uiMSG().question("Ignore output name warning?") ) return;

    mDynamicCastGet( EM::Fault3D*, destf3d, destfault );
    RefMan<EM::EMObject> tmpemobj = EM::FaultStickSet::create(EM::EMM());
    mDynamicCastGet( EM::FaultStickSet*, tmpfss, tmpemobj.ptr() );

    const bool merge = mCurItem( outputactcombo_, sKeyMergeWithExisting );

    if ( !destfault->isEmpty() && (!merge || destf3d) )
    {
	destfault->geometry().selectAllSticks( true );

	if ( merge )
	{
	    destf3d->geometry().copySelectedSticksTo(
			    tmpfss->geometry(), tmpfss->sectionID(0), false );
	}
	destfault->geometry().removeSelectedSticks( displayAfterwards() );
    }

    mDynamicCastGet( EM::FaultStickSet*, destfss, destfault );
    if ( !destfss )
    	destfss = tmpfss;

    const bool copy = mCurItem( transfercombo_, sKeyCopySelection );

    if ( copy )
	srcfault->geometry().selectStickDoubles( false, &destfss->geometry() );
    else
	srcfault->geometry().removeSelectedDoubles(true, &destfss->geometry());

    const bool adddestfsstohist = displayAfterwards() && destfss!=tmpfss;
    srcfault->geometry().copySelectedSticksTo( destfss->geometry(),
				    destfss->sectionID(0), adddestfsstohist );
    if ( destf3d )
    {
	EM::FSStoFault3DConverter::Setup setup;
	setup.addtohistory_ = displayAfterwards();
	if ( destf3d->isEmpty() )
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Auto;
	else if ( destf3d->geometry().areSticksVertical(destf3d->sectionID(0)) )
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Vertical;
	else
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Horizontal;

	EM::FSStoFault3DConverter fsstof3d( setup, *tmpfss, *destf3d );
	fsstof3d.convert( false );
    }

    if ( copy )
	srcfault->geometry().selectStickDoubles( false, &destfault->geometry());
    else
	srcfault->geometry().removeSelectedDoubles(true,&destfault->geometry());

    if ( curfssd_ )
	curfssd_->updateKnotMarkers();
    if ( curfltd_ )
	curfltd_->updateKnotMarkers();

    const int newnrselected = srcfault->geometry().nrSelectedSticks();
    if ( !copy && oldnrselected!=newnrselected )
	srcfault->setChangedFlag();

    destfault->setFullyLoaded( true );
    destfault->setPreferredColor( colorbutcolor_, true );
    displayUpdate();

    bool saved = false;
    if ( saveAfterwards() )
    {
	PtrMan<Executor> executor = destfault->saver();
	saved = executor->execute();
	if ( !saved )
	    uiMSG().error( "Cannot save output object" );
    }

    afterTransferUpdate();

    UndoEvent* undo = new FaultStickTransferUndoEvent( copy, saved );
    EM::EMM().undo().setUserInteractionEnd( EM::EMM().undo().addEvent(undo) );

    if ( newnrselected )
    {
	uiMSG().message( "Output object could not incorporate ",
		     toString(newnrselected), " of the selected sticks!" );
    }
}


void uiODFaultToolMan::afterTransferUpdate()
{
    randomcolor_ = getRandStdDrawColor();
    usercolorlink_.setEmpty();
    const bool clearname = mCurItem( outputactcombo_, sKeyCreateSingleNew );
    updateOutputItems( clearname );
    updateToolbarCB( 0 );
}


#define mGetDisplayVars( objsel, destmid, curid, sceneid ) \
\
    MultiID destmid = objsel->validKey(); \
\
    const int curid = curfltd_ ? curfltd_->id() : \
				 ( curfssd_ ? curfssd_->id() : -1 ); \
\
    const int sceneid = appl_.applMgr().visServer()->getSceneID( curid );


void uiODFaultToolMan::displayUpdate()
{
    mGetDisplayVars( getObjSel(), destmid, curid, sceneid );
    if ( destmid.isEmpty() || isOutputDisplayed() )
	return;

    if ( displayAfterwards() )
    {
	const EM::ObjectID destemid = EM::EMM().getObjectID( destmid );
	appl_.sceneMgr().addEMItem( destemid, sceneid );
	appl_.sceneMgr().updateTrees();
	appl_.applMgr().visServer()->setSelObjectId( curid );
    }
}


bool uiODFaultToolMan::isOutputDisplayed( uiSurfaceWrite* uisw ) const
{
    const uiIOObjSel* objsel = uisw ? uisw->getObjSel() : getObjSel();

    mGetDisplayVars( objsel, destmid, curid, sceneid );

    if ( destmid.isEmpty() || curid<0 || sceneid<0 )
	return false;

    for ( int idx=0; idx<displaycache_.size(); idx++ )
    {
	if ( displaycache_[idx]->mid_==destmid &&
	     displaycache_[idx]->sceneid_==sceneid )
	{
	    return displaycache_[idx]->isdisplayed_;
	}
    }
    displaycache_.insertAt( new DisplayCacheObj(), 0 );
    displaycache_[0]->mid_ = destmid;
    displaycache_[0]->sceneid_ = sceneid;
    displaycache_[0]->isdisplayed_ = false;

    TypeSet<int> destids;
    appl_.applMgr().visServer()->findObject( destmid, destids );

    for ( int idx=0; idx<destids.size(); idx++ )
    {
	if ( sceneid==appl_.applMgr().visServer()->getSceneID(destids[idx]) )
	{
	    displaycache_[0]->isdisplayed_ = true;
	    return true;
	}
    }
    return false;
}


bool uiODFaultToolMan::isInSerialMode() const
{ return mCurItem( outputactcombo_, sKeyCreateNewInSeries ); }


bool uiODFaultToolMan::isInCreateMode() const
{ return isInSerialMode() || mCurItem(outputactcombo_, sKeyCreateSingleNew); }


bool uiODFaultToolMan::displayAfterwards() const
{
    return isOutputDisplayed() ||
	   mGetSetting( displayAfterwards(), displayifnot_ );
}


bool uiODFaultToolMan::saveAfterwards() const
{
    return !isOutputDisplayed() ||
	   mGetSetting( saveAfterwards(), saveifdisplayed_ );
}


bool uiODFaultToolMan::randomColor() const
{
    return mGetSetting( colorMode(), colormode_ )
			== uiFaultStickTransferDlg::Random;
}


bool uiODFaultToolMan::inheritColor() const
{
    return mGetSetting( colorMode(), colormode_ )
			== uiFaultStickTransferDlg::Inherit;
}


bool uiODFaultToolMan::currentColor() const
{
    return mGetSetting( colorMode(), colormode_ )
			== uiFaultStickTransferDlg::Current;
}


bool uiODFaultToolMan::isOutputNameUsed( uiSurfaceWrite* uisw ) const
{
    const uiIOObjSel* objsel = uisw ? uisw->getObjSel() : getObjSel();

    if ( !objsel->existingTyped() )
	return false;

    const IOObj* ioobj = objsel->ioobj( true );
    if ( ioobj && ioobj->implExists(true) )
	return true;

    return isOutputDisplayed( uisw );
}


void uiODFaultToolMan::processOutputName()
{
    updateColorMode();
    outputColorChg( 0 );

    toolbar_->setSensitive( gobutidx_, true );
    BufferString objname = outputnamecombo_->text();

    if ( objname.isEmpty() )
    {
	toolbar_->setSensitive( gobutidx_, false );
    }
    else if ( !isInCreateMode() )
    {
	if ( !isOutputNameUsed() )
	{
	    BufferString tooltiptext = "Output name [of no existing Fault!]";
	    if ( !mCurItem(outputtypecombo_, sKeyToFault) )
		tooltiptext = "Output name [of no existing FaultStickSet!]";
	    outputnamecombo_->setToolTip( tooltiptext );
	    flashOutputName( true );
	    return;
	}
    }
    else if ( isInSerialMode() )
    {
	BufferString basenm = objname;
	int serialnr = removeSerialNumber( basenm );
	if ( serialnr<0 || mIsUdf(serialnr) )
	    serialnr = 1;

	BufferString serialname;
	for ( int idx=0; true; idx++ )
	{
	    serialname = basenm;
	    serialname += "_";
	    serialname += idx;
	    setAuxSurfaceWrite( serialname );

	    if ( !isOutputNameUsed(auxfaultwrite_) &&
		 !isOutputNameUsed(auxfsswrite_) )
	    {
		if ( idx >= serialnr )
		    break;
	    }
	    else if ( inheritColor() )
		outputColorChg( 0 );
	}

	updateColorMode();
	if ( !inheritColor() )
	    outputColorChg( 0 );

	if ( serialname != objname )
	{
	    flashOutputName( false, serialname );
	    return;
	}
    }
    else
    {
	BufferString basenm = objname;
	while ( removeSerialNumber(basenm) != -1 )
	    setAuxSurfaceWrite( basenm );

	const bool existingfault = isOutputNameUsed( auxfaultwrite_ );
	const bool existingfss = isOutputNameUsed( auxfsswrite_ );

	if (  existingfault || existingfss )
	{
	    BufferString tooltiptext = "Output name [of existing Fault!]";
	    if ( existingfss )
		tooltiptext = "Output name [of existing FaultStickSet!]";
	    outputnamecombo_->setToolTip( tooltiptext );
	    flashOutputName( true, basenm );
	    return;
	}
	else if ( basenm != objname )
	{
	    flashOutputName( false, basenm );
	    return;
	}
    }

    flashReset();
}


void uiODFaultToolMan::settingsToggleCB( CallBacker* )
{ showSettings( toolbar_->isOn(settingsbutidx_) ); }


void uiODFaultToolMan::settingsClosedCB( CallBacker* )
{ toolbar_->turnOn( settingsbutidx_, false ); }


void uiODFaultToolMan::surveyChg( CallBacker* )
{
    auxfaultwrite_->getObjSel()->inpBox()->setEmpty();
    auxfsswrite_->getObjSel()->inpBox()->setEmpty();

    setOutputName( "" );
    singlefaultitems_.erase();
    serialfaultitems_.erase();
    allfaultitems_.erase();
    singlefssitems_.erase();
    serialfssitems_.erase();
    allfssitems_.erase();

    publishOutputItems();
}


void uiODFaultToolMan::flashOutputName( bool error, const char* newname )
{
    //toolbar_->setSensitive( gobutidx_, !error );

    const BufferString name = newname ? newname : outputnamecombo_->text();
    const Color color = error ? Color(255,0,0) : Color(0,0,0);

    const bool doflash = toolbar_->isSensitive() &&
			 outputnamecombo_->sensitive() &&
			 (name!=flashname_ || color!=flashcolor_);
    
    flashname_ = name; flashcolor_ = color;

    if ( doflash )
    {
	flashtimer_.start( 500, true );
	outputnamecombo_->setBackgroundColor( flashcolor_ );
	mOutputNameComboSetTextColorSensitivityHack( Color(255,255,255) );
    }
    else
	flashOutputTimerCB( 0 );
}


void uiODFaultToolMan::flashOutputTimerCB( CallBacker* )
{
    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );
    outputnamecombo_->setBackgroundColor( Color(255,255,255) );
    setOutputName( flashname_ );
    updateColorMode();
    if ( !inheritColor() )
	outputColorChg( 0 );
}


void uiODFaultToolMan::flashReset()
{
    flashname_.setEmpty();
    flashcolor_ = Color(0,0,0);
    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );
    outputnamecombo_->setToolTip( "Output name" );
}


void uiODFaultToolMan::setOutputName( const char* outputname )
{
    NotifyStopper ns( outputnamecombo_->editTextChanged );
    outputnamecombo_->setText( outputname );
    setAuxSurfaceWrite( outputname );
}


void uiODFaultToolMan::setAuxSurfaceWrite( const char* outputname )
{
    auxfaultwrite_->getObjSel()->inpBox()->setText( outputname );
    auxfsswrite_->getObjSel()->inpBox()->setText( outputname );
}


void uiODFaultToolMan::undoCB( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo().unDo( 1, true  ) )
	uiMSG().error("Could not undo everything.");
    EM::EMM().burstAlertToAll( false );
    updateToolbarCB( 0 );
}


void uiODFaultToolMan::redoCB( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo().reDo( 1, true  ) )
	uiMSG().error("Could not redo everything.");
    EM::EMM().burstAlertToAll( false );
    updateToolbarCB( 0 );
}


static void keyDown( bool yn )
{
    if ( uiMain::keyboardEventHandler().event().key_ == OD::Space )
	MPE::ObjectEditor::enableNodeCloning( yn );
}


void uiODFaultToolMan::keyPressedCB( CallBacker* )
{ keyDown( true ); }


void uiODFaultToolMan::keyReleasedCB( CallBacker* )
{ keyDown( false ); }
