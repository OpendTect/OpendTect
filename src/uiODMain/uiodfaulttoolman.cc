/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Jaap Glas
 Date:		December 2009
___________________________________________________________________

-*/


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
#include "uipixmap.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uivispartserv.h"
#include "uiodmenumgr.h"
#include "od_helpids.h"


uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiODMain& appl,
						  const Setup& su )
    : uiDialog( &appl, uiDialog::Setup(tr("Faultstick transfer"),
				       tr("Transfer settings"),
				      mODHelpKey(mFaultStickTransferDlgHelpID))
					.modal(false) )
    , displayifnot_( su.displayifnot_ )
    , saveifdisplayed_( su.saveifdisplayed_ )
    , colormodechg( this )
{
    setCtrlStyle( CloseOnly );

    uiLabel* colormodelbl = new uiLabel( this,
				uiStrings::phrOutput(tr("color selection") ) );

    BufferStringSet serialbss; serialbss.add( "Inherit" );
    serialbss.add( "Random" ); serialbss.add( "User-defined" );
    serialcolormodefld_ = new uiGenInput(this,uiString::emptyString(),
					 StringListInpSpec(serialbss));
    serialcolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    serialcolormodefld_->attach( alignedBelow, colormodelbl );

    BufferStringSet existsbss;
    existsbss.add( "Current" ); existsbss.add( "User-defined" );
    existscolormodefld_ = new uiGenInput(this,uiString::emptyString(),
					 StringListInpSpec(existsbss));
    existscolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    existscolormodefld_->attach( alignedBelow, colormodelbl );

    BufferStringSet singlebss; singlebss.add( "User-defined" );
    singlecolormodefld_ = new uiGenInput(this,uiString::emptyString(),
					 StringListInpSpec(singlebss));
    singlecolormodefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,colorModeChg) );
    singlecolormodefld_->attach( alignedBelow, colormodelbl );

    setColorMode( su.colormode_ );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, serialcolormodefld_ );

    uiLabel* aftercopymovelbl = new uiLabel( this, tr("After copy / move:") );
    aftercopymovelbl->setMinimumWidth( 250 );
    aftercopymovelbl->attach( alignedBelow, horsep );

    displayfld_ = new uiCheckBox( this, uiStrings::sDisplay() );
    displayfld_->setChecked( displayifnot_ );
    displayfld_->attach( alignedBelow, aftercopymovelbl );
    displayfld_->activated.notify(mCB(this,uiFaultStickTransferDlg,displayCB));

    savefld_ = new uiCheckBox( this, uiStrings::sSave() );
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
{ mODTextTranslationClass(FaultStickTransferUndoEvent);
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
	    uiMSG().message( tr("Will not undo saving the output file!") );
	return true;
    }

    bool reDo()
    { return true; }

protected:
    bool copy_;
    bool saved_;
};


//============================================================================

uiString uiODFaultToolMan::sKeyCopySelection()
{ return tr( "Copy Selection to" ); }


uiString uiODFaultToolMan::sKeyMoveSelection()
{ return tr( "Move Selection to" ); }


uiString uiODFaultToolMan::sKeyToFault()
{ return uiStrings::sFault(); }


uiString uiODFaultToolMan::sKeyToFaultStickSet()
{ return uiStrings::sFaultStickSet(); }


uiString uiODFaultToolMan::sKeyCreateSingleNew()
{ return tr( "Create Single New" ); }


uiString uiODFaultToolMan::sKeyCreateNewInSeries()
{ return tr( "Create New in Series" ); }


uiString uiODFaultToolMan::sKeyMergeWithExisting()
{ return tr( "Merge with Existing" ); }


uiString uiODFaultToolMan::sKeyReplaceExisting()
{ return tr( "Replace Existing" ); }

#define mCurItem( combo, key ) \
    ( BufferString(key().getFullString()) == combo->text() )


uiODFaultToolMan::uiODFaultToolMan( uiODMain& appl )
    : appl_(appl)
    , tracktbwashidden_(false)
    , selectmode_(false)
    , settingsdlg_(nullptr)
    , colorbutcolor_(OD::Color(0,0,0))
    , usercolor_(OD::Color(0,0,255))
    , randomcolor_(OD::getRandStdDrawColor())
    , flashcolor_(OD::Color(0,0,0))
    , curemid_(-1)
{
    toolbar_ = new uiToolBar( &appl_,
	    tr("Fault Stick Control"),uiToolBar::Bottom);
    editbutidx_ = toolbar_->addButton( "editsticks", tr("Edit Sticks"),
				mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    selbutidx_ = toolbar_->addButton( "selectsticks", tr("Select Sticks"),
				mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    toolbar_->addSeparator();

    transfercombo_ = new uiComboBox( toolbar_, "Stick transfer action" );
    transfercombo_->setToolTip( tr("Stick transfer action") );
    transfercombo_->addItem( sKeyCopySelection() );
    transfercombo_->addItem( sKeyMoveSelection() );
    toolbar_->addObject( transfercombo_ );

    outputtypecombo_ = new uiComboBox( toolbar_, "Output type" );
    outputtypecombo_->setToolTip( tr("Output type") );
    outputtypecombo_->addItem( sKeyToFault() );
    outputtypecombo_->addItem( sKeyToFaultStickSet() );
    outputtypecombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputTypeChg) );
    toolbar_->addObject( outputtypecombo_ );

    outputactcombo_ = new uiComboBox( toolbar_, "Output operation" );
    outputactcombo_->setToolTip( tr("Output operation"));
    outputactcombo_->addItem( sKeyCreateSingleNew() );
    outputactcombo_->addItem( sKeyCreateNewInSeries() );
    outputactcombo_->addSeparator();
    outputactcombo_->addItem( sKeyMergeWithExisting() );
    outputactcombo_->addItem( sKeyReplaceExisting() );
    outputactcombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputActionChg) );
    toolbar_->addObject( outputactcombo_ );

    outputnamecombo_ = new uiComboBox( toolbar_, "Output name" );
    outputnamecombo_->setToolTip( tr("Output name"));
    outputnamecombo_->setReadOnly( false );
    outputnamecombo_->setMinimumWidth( 150 );
    outputnamecombo_->editTextChanged.notify(
				mCB(this,uiODFaultToolMan,outputEditTextChg) );
    outputnamecombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputComboSelChg) );
    outputnamecombo_->addItem( uiString::emptyString() );
    toolbar_->addObject( outputnamecombo_ );

    auxfaultwrite_ = new uiSurfaceWrite( &appl_,
	    uiSurfaceWrite::Setup(EMFault3DTranslatorGroup::sGroupName(),
				  EMFault3DTranslatorGroup::sTypeName()));
    auxfaultwrite_->getObjSel()->setConfirmOverwrite( false );
    auxfaultwrite_->getObjSel()->selectionDone.notify(
				mCB(this,uiODFaultToolMan,outputSelectedCB) );
    auxfsswrite_ = new uiSurfaceWrite( &appl_,
	uiSurfaceWrite::Setup(EMFaultStickSetTranslatorGroup::sGroupName(),
			      EMFaultStickSetTranslatorGroup::sTypeName()));
    auxfsswrite_->getObjSel()->setConfirmOverwrite( false );
    auxfsswrite_->getObjSel()->selectionDone.notify(
				mCB(this,uiODFaultToolMan,outputSelectedCB) );

    outputselbut_ = new uiPushButton( toolbar_, uiStrings::sSelect(),
				mCB(this,uiODFaultToolMan,selectOutputCB),
				false );
    outputselbut_->setToolTip( tr("Select output") );
    toolbar_->addObject( outputselbut_ );

    colorbut_ = new uiToolButton( toolbar_, "empty", tr("Output Color"),
				mCB(this,uiODFaultToolMan,colorPressedCB) );
    colorbut_->setToolTip( colorbut_->text() );
    toolbar_->addObject( colorbut_ );

    settingsbutidx_ = toolbar_->addButton("tools", tr("More Transfer Settings"),
				mCB(this,uiODFaultToolMan,settingsToggleCB),
				true );

    gobutidx_ = toolbar_->addButton( "ok",
				tr("Transfer Selected Sticks"),
				mCB(this,uiODFaultToolMan,transferSticksCB),
				false );

    toolbar_->addSeparator();

    removalbutidx_ = toolbar_->addButton( "trashcan",
				tr("Remove Selected Sticks"),
				mCB(this,uiODFaultToolMan,stickRemovalCB),
				false );
    toolbar_->addSeparator();


    undobutidx_ = toolbar_->addButton( "undo", uiStrings::sUndo(),
				mCB(this,uiODFaultToolMan,undoCB), false );

    redobutidx_ = toolbar_->addButton( "redo", uiStrings::sRedo(),
				mCB(this,uiODFaultToolMan,redoCB), false );

    toolbar_->addSeparator();

    mAttachCB( visBase::DM().selMan().selnotifier,
	       uiODFaultToolMan::treeItemSelCB );
    mAttachCB( visBase::DM().selMan().deselnotifier,
	       uiODFaultToolMan::treeItemDeselCB );
    mAttachCB( EM::EMM().addRemove,
	       uiODFaultToolMan::addRemoveEMObjCB );
    mAttachCB( appl_.applMgr().visServer()->objectAdded,
	       uiODFaultToolMan::addRemoveVisObjCB );
    mAttachCB( appl_.applMgr().visServer()->objectRemoved,
	       uiODFaultToolMan::addRemoveVisObjCB );
    mAttachCB( appl_.postFinalize(),
	       uiODFaultToolMan::finalizeDoneCB );
    mAttachCB( deseltimer_.tick,
	       uiODFaultToolMan::deselTimerCB );
    mAttachCB( editreadytimer_.tick,
	       uiODFaultToolMan::editReadyTimerCB );
    mAttachCB( flashtimer_.tick,
	       uiODFaultToolMan::flashOutputTimerCB );
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       uiODFaultToolMan::keyPressedCB );
    mAttachCB( uiMain::keyboardEventHandler().keyReleased,
	       uiODFaultToolMan::keyReleasedCB );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    detachAllNotifiers();
    delete appl_.removeToolBar( toolbar_ );
    if ( settingsdlg_ )
	delete settingsdlg_;
}


void uiODFaultToolMan::finalizeDoneCB( CallBacker* )
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
	curemid_ = curfssd_ ? curfssd_->getEMObjectID()
			    : curfltd_->getEMObjectID();

	const EM::EMObject* emobj = EM::EMM().getObject( curemid_ );
	if ( !emobj || emobj->isEmpty() )
	{
	    toolbar_->turnOn( selbutidx_, false );
	    selectmode_ = false;
	}
	else
	{
	    mAttachCBIfNotAttached( EM::EMM().undo(emobj->id()).undoredochange,
		uiODFaultToolMan::updateToolbarCB );
	}
	editSelectToggleCB( 0 );
	if ( areSticksAccessible() )
	    appl_.applMgr().visServer()->setViewMode( false );

	appl_.applMgr().visServer()->setCurInterObjID( selid );
	processOutputName();
	enableToolbar( true );

	mAttachCBIfNotAttached( IOM().surveyChanged,
				uiODFaultToolMan::surveyChg );

	if ( curfssd_ )
	{
	    mAttachCB( curfssd_->displaymodechange,
		       uiODFaultToolMan::displayModeChg );
	}
	if ( curfltd_ )
	{
	    mAttachCB( curfltd_->displaymodechange,
		       uiODFaultToolMan::displayModeChg );
	}
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
	if ( curfssd_ )
	{
	    mDetachCB( curfssd_->displaymodechange,
		       uiODFaultToolMan::displayModeChg );
	}
	if ( curfltd_ )
	{
	    mDetachCB( curfltd_->displaymodechange,
		       uiODFaultToolMan::displayModeChg );
	}

	deseltimer_.start( 100, true );
    }
}


void uiODFaultToolMan::addRemoveEMObjCB( CallBacker* )
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


void uiODFaultToolMan::addRemoveVisObjCB( CallBacker* )
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
	    outputnamecombo_->sensitive() ? color : OD::Color(128,128,128) );

void uiODFaultToolMan::enableToolbar( bool yn )
{
    if ( yn == toolbar_->isSensitive() )
	return;

    if ( yn )
    {
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
    }
}


void uiODFaultToolMan::showSettings( bool yn )
{
    if ( !settingsdlg_ )
    {

	if ( yn )
	{
	    settingsdlg_ = new uiFaultStickTransferDlg(appl_, settingssetup_ );
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
{
    if ( curfssd_ )
	return !curfssd_->areAllKnotsHidden();

    return curfltd_ && curfltd_->areSticksDisplayed();
}


void uiODFaultToolMan::enableStickAccess( bool yn )
{
    if ( curfssd_ && curfssd_->areAllKnotsHidden()==yn )
	curfssd_->hideAllKnots( !yn );

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

    if ( cb )
	appl_.applMgr().visServer()->setViewMode( !areSticksAccessible() );

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
    const EM::EMObject* emobj = EM::EMM().getObject(curemid_);

    if ( !emobj )
	return;

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

    toolbar_->setSensitive(undobutidx_, EM::EMM().undo(emobj->id()).canUnDo());
    uiString undotooltip;
    if ( EM::EMM().undo(emobj->id()).canUnDo() )
    undotooltip = tr("Undo %1").arg( EM::EMM().undo(emobj->id()).unDoDesc() );
    else
	undotooltip = uiStrings::sUndo();

    toolbar_->setToolTip( undobutidx_, undotooltip );

    toolbar_->setSensitive(redobutidx_, EM::EMM().undo(emobj->id()).canReDo());
    uiString redotooltip;
    if ( EM::EMM().undo(emobj->id()).canReDo() )
	redotooltip =
	tr( "Redo %1").arg( EM::EMM().undo(emobj->id()).reDoDesc() );
    else
	redotooltip = uiStrings::sRedo();

    toolbar_->setToolTip( redobutidx_, redotooltip );
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
    mDefineStaticLocalObject( uiPoint, lastpos, (uiPoint::udf()) );

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
    char* lastptr = objname.getCStr() + objname.size() - 1;
    char* ptr = lastptr;

    while ( ptr>=objname.buf() && iswdigit(*ptr) )
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
    outputnamecombo_->addItem( uiString::emptyString() );
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
    colorbut_->setToolTip( tr("Output color [user-defined]") );

    if ( cb )
    {
	usercolor_ = colorbutcolor_;
	usercolorlink_ = getObjSel()->inpBox()->text();
	updateColorMode();
    }
    else if ( randomColor() )
    {
	colorbutcolor_ = randomcolor_;
	colorbut_->setToolTip( tr("Output color [random]") );
    }
    else
    {
	colorbutcolor_ = usercolor_;

	if ( currentColor() || inheritColor() )
	{
	    MultiID mid = auxfaultwrite_->getObjSel()->validKey();
	    if ( !isOutputNameUsed(auxfaultwrite_) )
		mid = auxfsswrite_->getObjSel()->validKey();

	    if ( !mid.isUdf() )
	    {
		const EM::ObjectID emid  = EM::EMM().getObjectID( mid );
		const EM::EMObject* emobj = EM::EMM().getObject( emid );

		IOPar iopar;
		OD::Color curcolor;
		if ( emobj )
		    curcolor = emobj->preferredColor();
		else
		    EM::EMM().readDisplayPars( mid, iopar );

		if ( emobj || iopar.get(sKey::Color(),curcolor) )
		{
		    colorbutcolor_ = curcolor;
		    colorbut_->setToolTip(
			currentColor() ? tr("Output color [current]")
				       : tr("Output color [predecessor]") );
		}
	    }
	}
    }

    uiPixmap colorpm( 20, 20 );
    colorpm.fill( colorbutcolor_ );
    colorbut_->setPixmap( colorpm );
}


void uiODFaultToolMan::colorModeChg( CallBacker* cb )
{
    const bool userdef = !randomColor() && !inheritColor() && !currentColor();

    usercolorlink_ = userdef ? getObjSel()->inpBox()->text() : "";

    if ( randomColor() )
	randomcolor_ = OD::getRandStdDrawColor();

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
	uiMSG().error( tr("No selected fault stick(s) to remove") );
	return;
    }

    srcfault->geometry().removeSelectedSticks( true );
    srcfault->setChangedFlag();
    EM::EMM().undo(srcemobj->id()).setUserInteractionEnd(
	EM::EMM().undo(srcemobj->id()).currentEventID());
}


void uiODFaultToolMan::transferSticksCB( CallBacker* )
{
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
	uiMSG().error( tr("No selected fault stick(s) to transfer") );
	return;
    }

    const MultiID destmid = getObjSel()->key();
    if ( destmid.isUdf() )
	return;

    EM::EMM().loadIfNotFullyLoaded( destmid );
    const EM::ObjectID destemid = EM::EMM().getObjectID( destmid );
    RefMan<EM::EMObject> destemobj = EM::EMM().getObject( destemid );
    mDynamicCastGet( EM::Fault*, destfault, destemobj.ptr() );
    if ( !destfault )
	return;

    if ( destfault == srcfault )
    {
	uiMSG().error( tr("No use to transfer selected sticks to myself") );
	return;
    }

    if ( flashcolor_==OD::Color(255,0,0) &&
	 !uiMSG().question(tr("Ignore output name warning?")) ) return;

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
	else if ( destf3d->geometry().areSticksVertical(destf3d->sectionID(0)))
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Vertical;
	else
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Horizontal;

	EM::FSStoFault3DConverter fsstof3d( setup, *tmpfss, *destf3d );
	fsstof3d.convert( false );
    }

    if ( copy )
	srcfault->geometry().selectStickDoubles(false, &destfault->geometry());
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
	    uiMSG().error( tr("Cannot save output object") );
    }

    afterTransferUpdate();

    UndoEvent* undo = new FaultStickTransferUndoEvent( copy, saved );
    EM::EMM().undo(destfault->id()).setUserInteractionEnd(
	EM::EMM().undo(destfault->id()).addEvent(undo) );

    if ( newnrselected )
    {
	uiMSG().message(tr("Output object could not incorporate %1"
			   " of the selected sticks!")
		      .arg(newnrselected));
    }
}


void uiODFaultToolMan::afterTransferUpdate()
{
    randomcolor_ = OD::getRandStdDrawColor();
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
    if ( destmid.isUdf() || isOutputDisplayed() )
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

    if ( destmid.isUdf() || curid<0 || sceneid<0 )
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
	    uiString tooltiptext = tr("Output name [of no existing %1!]");
	    if ( mCurItem(outputtypecombo_, sKeyToFault) )
		tooltiptext.arg( sKeyToFault() );
	    else
		tooltiptext.arg( sKeyToFaultStickSet() );
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
	uiString tooltiptext = tr("Output name [of existing Fault!]");
	    if ( existingfss )
	tooltiptext = tr("Output name [of existing FaultStickSet!]");
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
    const OD::Color color = error ? OD::Color(255,0,0) : OD::Color(0,0,0);

    const bool doflash = toolbar_->isSensitive() &&
			 outputnamecombo_->sensitive() &&
			 (name!=flashname_ || color!=flashcolor_);

    flashname_ = name; flashcolor_ = color;

    if ( doflash )
    {
	flashtimer_.start( 500, true );
	outputnamecombo_->setBackgroundColor( flashcolor_ );
	mOutputNameComboSetTextColorSensitivityHack( OD::Color(255,255,255) );
    }
    else
	flashOutputTimerCB( 0 );
}


void uiODFaultToolMan::flashOutputTimerCB( CallBacker* )
{
    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );
    outputnamecombo_->setBackgroundColor( OD::Color(255,255,255) );
    setOutputName( flashname_ );
    updateColorMode();
    if ( !inheritColor() )
	outputColorChg( 0 );
}


void uiODFaultToolMan::flashReset()
{
    flashname_.setEmpty();
    flashcolor_ = OD::Color(0,0,0);
    mOutputNameComboSetTextColorSensitivityHack( flashcolor_ );
    outputnamecombo_->setToolTip(
	    uiStrings::phrOutput( uiStrings::sName().toLower() ) );
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
    const EM::EMObject* curemobj = EM::EMM().getObject(curemid_);
    if ( !curemobj )
	return;

    if ( !curfltd_ && !curfssd_ ) return;
    MouseCursorChanger mcc( MouseCursor::Wait );
    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo(curemobj->id()).unDo( 1, true  ) )
	uiMSG().error(tr("Could not undo everything."));
    EM::EMM().burstAlertToAll( false );
    updateToolbarCB( 0 );
    uiMain::keyboardEventHandler().setHandled( true );
}


void uiODFaultToolMan::redoCB( CallBacker* )
{
    const EM::EMObject* curemobj = EM::EMM().getObject(curemid_);
    if ( !curemobj )
	return;

    if ( !curfltd_ && !curfssd_ ) return;

    MouseCursorChanger mcc( MouseCursor::Wait );
    EM::EMM().burstAlertToAll( true );
    if ( !EM::EMM().undo(curemobj->id()).reDo( 1, true  ) )
	uiMSG().error(tr("Could not redo everything."));
    EM::EMM().burstAlertToAll( false );
    updateToolbarCB( 0 );
    uiMain::keyboardEventHandler().setHandled( true );
}


static void keyDown( bool yn )
{
    if ( uiMain::keyboardEventHandler().hasEvent() &&
	uiMain::keyboardEventHandler().event().key_ == '`' )
	MPE::ObjectEditor::enableNodeCloning( yn );
}


void uiODFaultToolMan::keyPressedCB( CallBacker* )
{
    keyDown( true );
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();

    if ( KeyboardEvent::isUnDo(kbe) )
	undoCB( 0 );

    if ( KeyboardEvent::isReDo(kbe) )
	redoCB( 0 );

}


void uiODFaultToolMan::keyReleasedCB( CallBacker* )
{ keyDown( false ); }
