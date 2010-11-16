/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttoolman.cc,v 1.19 2010-11-16 11:27:27 cvsbert Exp $";


#include "uiodfaulttoolman.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "randcolor.h"
#include "timefun.h"
#include "undo.h"
#include "visdataman.h"
#include "visfaultsticksetdisplay.h"
#include "visfaultdisplay.h"
#include "visselman.h"

#include "uitoolbutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "pixmap.h"


static void comboCopy( const uiComboBox& from, uiComboBox& to )
{
    if ( !to.editTextChanged.isEnabled() )
	return;

    to.editTextChanged.disable();
    to.setEmpty();
    for ( int idx=0; idx<from.size(); idx++ )
	to.addItem( from.textOfItem(idx) );

    to.setText( from.text() );
    to.editTextChanged.enable();
    to.editTextChanged.trigger();
}


uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiODMain& appl,
						  const Setup& su,
						  uiODFaultToolMan* ftbman )
    : uiDialog( &appl, uiDialog::Setup("Faultstick transfer",
				 "Transfer settings","104.1.8").modal(false) )
    , appl_( appl )
    , ftbman_( ftbman )
    , displayifnot_( su.displayifnot_ )
    , saveifdisplayed_( su.saveifdisplayed_ )
{
    setCtrlStyle( LeaveOnly );

    outtypefld_ = new uiGenInput( this, "Output type",
		      BoolInpSpec(su.outputfault_,"Fault","FaultStickSet") );
    outtypefld_->valuechanged.notify(
			mCB(this,uiFaultStickTransferDlg,outputTypeChg) );

    faultoutputfld_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(EMFault3DTranslatorGroup::keyword()));
    faultoutputfld_->attach( alignedBelow, outtypefld_ );
    faultoutputfld_->getObjSel()->setConfirmOverwrite( false );
    faultoutputfld_->getObjSel()->inpBox()->editTextChanged.notify(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );

    fssoutputfld_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(EMFaultStickSetTranslatorGroup::keyword()));
    fssoutputfld_->attach( alignedBelow, outtypefld_ );
    fssoutputfld_->getObjSel()->setConfirmOverwrite( false );
    fssoutputfld_->getObjSel()->inpBox()->editTextChanged.notify(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );

    colorfld_ = new uiColorInput( this,
		    uiColorInput::Setup(Color(0,0,0)).lbltxt("Current color"),
				  "Output color" );
    colorfld_->colorChanged.notify(
			mCB(this,uiFaultStickTransferDlg,outputColorChg) );
    colorfld_->attach( alignedBelow, faultoutputfld_ );

    uiLabel* aftertransferlbl = new uiLabel( this, "After transfer:" );
    aftertransferlbl->attach( alignedBelow, colorfld_ );

    displayfld_ = new uiCheckBox( this, "display" );
    displayfld_->setChecked( displayifnot_ );
    displayfld_->attach( rightOf, aftertransferlbl );
    displayfld_->activated.notify(mCB(this,uiFaultStickTransferDlg,displayCB));

    savefld_ = new uiCheckBox( this, "save" );
    savefld_->setChecked( saveifdisplayed_ );
    savefld_->attach( rightOf, displayfld_ );
    savefld_->activated.notify( mCB(this,uiFaultStickTransferDlg,saveCB) );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, aftertransferlbl );

    sequelnamefld_ = new uiCheckBox( this,"Generate sequel name" );
    sequelnamefld_->setChecked( su.sequelnaming_ );
    sequelnamefld_->activated.notify(
			mCB(this,uiFaultStickTransferDlg,sequelNameCB) );
    sequelnamefld_->attach( ensureBelow, horsep );
    sequelnamefld_->attach( alignedBelow, aftertransferlbl );

    sequelcolorfld_ = new uiGenInput( this, "Sequel color",
		      BoolInpSpec(!su.colorrandom_,"Unchanged","Random") );
    sequelcolorfld_->attach( alignedBelow, sequelnamefld_ );

    finaliseDone.notify( mCB(this,uiFaultStickTransferDlg,finaliseDoneCB) );
    appl_.applMgr().visServer()->objectaddedremoved.notify(
			mCB(this,uiFaultStickTransferDlg,displayChg) );
}


uiFaultStickTransferDlg::~uiFaultStickTransferDlg()
{
    outtypefld_->valuechanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputTypeChg) );
    faultoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );
    fssoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );
    colorfld_->colorChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputColorChg) );

    displayfld_->activated.remove(mCB(this,uiFaultStickTransferDlg,displayCB));
    savefld_->activated.remove( mCB(this,uiFaultStickTransferDlg,saveCB) );

    sequelnamefld_->activated.remove(
			mCB(this,uiFaultStickTransferDlg,sequelNameCB) );

    finaliseDone.remove( mCB(this,uiFaultStickTransferDlg,finaliseDoneCB) );
    appl_.applMgr().visServer()->objectaddedremoved.remove(
			mCB(this,uiFaultStickTransferDlg,displayChg) );
}


void uiFaultStickTransferDlg::finaliseDoneCB( CallBacker* )
{
    outputTypeChg( 0 );
    sequelNameCB( 0 ); 
}


void uiFaultStickTransferDlg::outputTypeChg( CallBacker* cb )
{
    faultoutputfld_->display( outtypefld_->getBoolValue() );
    fssoutputfld_->display( !outtypefld_->getBoolValue() );
    if ( cb )
	outputComboChg( 0 );
}


void uiFaultStickTransferDlg::outputComboChg( CallBacker* cb )
{
    NotifyStopper ns( getObjSel()->inpBox()->editTextChanged );
    comboCopy( *getObjSel()->inpBox(), *ftbman_->getOutputCombo() );
    displayChg( 0 );
}


void uiFaultStickTransferDlg::outputColorChg( CallBacker* )
{
    NotifyStopper ns( colorfld_->colorChanged );
    ftbman_->getOutputColor()->setColor( colorfld_->color() );
    ftbman_->getOutputColor()->colorChanged.trigger();
}


void uiFaultStickTransferDlg::displayCB( CallBacker* )
{
    displayifnot_ = displayfld_->isChecked();
    displayChg( 0 );
}


void uiFaultStickTransferDlg::saveCB( CallBacker* )
{ saveifdisplayed_ = savefld_->isChecked(); }


void uiFaultStickTransferDlg::displayChg( CallBacker* )
{
    NotifyStopper displaynotifystopper( displayfld_->activated );
    NotifyStopper savenotifystopper( savefld_->activated );

    displayfld_->setSensitive( !ftbman_->isOutputDisplayed() );
    displayfld_->setChecked( ftbman_->isOutputDisplayed() || displayifnot_ );
    savefld_->setSensitive( displayfld_->isChecked() );
    savefld_->setChecked( !displayfld_->isChecked() || saveifdisplayed_ );
}


void uiFaultStickTransferDlg::sequelNameCB( CallBacker* cb )
{ sequelcolorfld_->setSensitive( sequelnamefld_->isChecked() ); }


uiIOObjSel* uiFaultStickTransferDlg::getObjSel()
{
    if ( outtypefld_->getBoolValue() )
	return faultoutputfld_->getObjSel();
    else
	return fssoutputfld_->getObjSel();
}


uiColorInput* uiFaultStickTransferDlg::getOutputColor()
{ return colorfld_; }


bool uiFaultStickTransferDlg::displayAfterwards() const
{ return displayfld_->isChecked(); }


bool uiFaultStickTransferDlg::saveAfterwards() const
{ return savefld_->isChecked(); }


bool uiFaultStickTransferDlg::generateSequelName() const
{ return sequelnamefld_->isChecked(); }


bool uiFaultStickTransferDlg::randomSequelColor() const
{ return !sequelcolorfld_->getBoolValue(); }


void uiFaultStickTransferDlg::setOutputFields( const uiComboBox& faultcombo,
					       const uiComboBox& fsscombo )
{
    comboCopy( faultcombo, *faultoutputfld_->getObjSel()->inpBox() );
    comboCopy( fsscombo, *fssoutputfld_->getObjSel()->inpBox() );
}


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


#define mGetSetting( settingsfunc, setupmember ) \
( settingsdlg_ ? settingsdlg_->settingsfunc : settingssetup_.setupmember )


uiODFaultToolMan::uiODFaultToolMan( uiODMain& appl )
    : appl_( appl )
    , tracktbwashidden_( false )
    , selectmode_( false )
    , settingsdlg_( 0 )
    , newcolor_( getRandStdDrawColor() )
{
    toolbar_ = new uiToolBar( &appl_, "Fault stick control", uiToolBar::Bottom);
    editbutidx_ = toolbar_->addButton( "editsticks.png", "Edit sticks",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    selbutidx_ = toolbar_->addButton( "selectsticks.png", "Select sticks",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				true );
    toolbar_->addSeparator();

    removalbutidx_ = toolbar_->addButton( "removesticks.png",
	    			"Remove selected sticks",
	    			mCB(this,uiODFaultToolMan,stickRemovalCB),
				false );

    copybutidx_ = toolbar_->addButton( "copysticks.png",
	    			"Copy selected sticks",
	    			mCB(this,uiODFaultToolMan,stickCopyCB), false );

    movebutidx_ = toolbar_->addButton( "movesticks.png",
	    			"Move selected sticks",
	    			mCB(this,uiODFaultToolMan,stickMoveCB), false );
    toolbar_->addSeparator();

    tboutputcombo_ = new uiComboBox( toolbar_, "Output name" );
    tboutputcombo_->setToolTip( tboutputcombo_->name() );
    tboutputcombo_->setReadOnly( false );
    tboutputcombo_->setMinimumWidth( 150 );
    tboutputcombo_->editTextChanged.notify(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    toolbar_->addObject( tboutputcombo_ );

    manfaultoutput_ = new uiSurfaceWrite( 0, 
	    uiSurfaceWrite::Setup(EMFault3DTranslatorGroup::keyword()));
    manfaultoutput_->getObjSel()->setConfirmOverwrite( false );
    manfssoutput_ = new uiSurfaceWrite( 0, 
	    uiSurfaceWrite::Setup(EMFaultStickSetTranslatorGroup::keyword()));
    manfssoutput_->getObjSel()->setConfirmOverwrite( false );

    manoutputcolor_ = new uiColorInput( 0, uiColorInput::Setup(Color(0,0,0)),
				       "Output color" );
    manoutputcolor_->colorChanged.notify(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );

    tbcolorbutton_ = new uiToolButton( toolbar_, uiIcon::None(),
	    "Output color", mCB(this,uiODFaultToolMan,colorPressedCB) );
    toolbar_->addButton( tbcolorbutton_ );

    settingsbutidx_ = toolbar_->addButton( "faulttoolsettings.png",
	    			"Transfer settings",
	    			mCB(this,uiODFaultToolMan,settingsToggleCB),
				true );
    toolbar_->addSeparator();

    undobutidx_ = toolbar_->addButton( "undo.png", "Undo",
	    			mCB(this,uiODFaultToolMan,undoCB), false );

    redobutidx_ = toolbar_->addButton( "redo.png", "Redo",
	    			mCB(this,uiODFaultToolMan,redoCB), false );

    toolbar_->addSeparator();

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );

    appl_.finaliseDone.notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.notify( mCB(this,uiODFaultToolMan,deselTimerCB) );
    EM::EMM().undo().changenotifier.notify(
				mCB(this,uiODFaultToolMan,updateToolbarCB) );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    tboutputcombo_->editTextChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    manoutputcolor_->colorChanged.remove(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );

    visBase::DM().selMan().selnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.remove( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    appl_.finaliseDone.remove( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.remove( mCB(this,uiODFaultToolMan,deselTimerCB) );
    IOM().surveyChanged.remove( mCB(this,uiODFaultToolMan,surveyChg) );
    EM::EMM().undo().changenotifier.remove(
				mCB(this,uiODFaultToolMan,updateToolbarCB) );

    delete toolbar_;
    delete manfaultoutput_;
    delete manfssoutput_;
    delete manoutputcolor_;

    if ( settingsdlg_ )
    {
	settingsdlg_->windowClosed.remove(
				mCB(this,uiODFaultToolMan,settingsClosedCB) );
	delete settingsdlg_;
    }
}


void uiODFaultToolMan::finaliseDoneCB( CallBacker* )
{
    clearCurDisplayObj();
}


uiToolBar* uiODFaultToolMan::getToolBar()
{ return toolbar_; }


uiComboBox* uiODFaultToolMan::getOutputCombo()
{ return tboutputcombo_; }


uiColorInput* uiODFaultToolMan::getOutputColor()
{ return manoutputcolor_; }


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
	enableToolbar( true );
	comboCopy( *getObjSel()->inpBox(), *tboutputcombo_ );

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
    if ( !EM::EMM().getObject(curemid_) )
	deseltimer_.start( 100, true );
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

    if ( !yn )
    {
	showSettings( false );
	appl_.applMgr().visServer()->turnSelectionModeOn( false );

	if ( tracktb_->isHidden() )
	    tracktb_->display( !tracktbwashidden_);
    }
}


void uiODFaultToolMan::showSettings( bool yn )
{
    if ( !settingsdlg_ )
    {

	if ( yn )
	{
	    settingsdlg_ = new uiFaultStickTransferDlg( appl_,
							settingssetup_, this );
	    settingsdlg_->windowClosed.notify(
				mCB(this,uiODFaultToolMan,settingsClosedCB) );
	    settingsdlg_->go();
	    settingsdlg_->setOutputFields(
				*manfaultoutput_->getObjSel()->inpBox(),
				*manfssoutput_->getObjSel()->inpBox() );
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

    toolbar_->setSensitive( removalbutidx_, selecting );
    toolbar_->setSensitive( copybutidx_, selecting );
    toolbar_->setSensitive( movebutidx_, selecting );
    toolbar_->setSensitive( settingsbutidx_, selecting );
    tboutputcombo_->setSensitive( selecting );
    tbcolorbutton_->setSensitive( selecting );

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
    if ( settingsdlg_ )
	return settingsdlg_->getObjSel();
    else if ( settingssetup_.outputfault_ )
	return manfaultoutput_->getObjSel();
    else 
	return manfssoutput_->getObjSel();
}


const uiIOObjSel* uiODFaultToolMan::getObjSel() const
{ return const_cast<uiODFaultToolMan*>(this)->getObjSel(); }


void uiODFaultToolMan::outputComboChg( CallBacker* cb )
{
    NotifyStopper ns( tboutputcombo_->editTextChanged );
    comboCopy( *tboutputcombo_, *getObjSel()->inpBox() );
    outputColorChg( 0 );
}


void uiODFaultToolMan::colorPressedCB( CallBacker* cb )
{ manoutputcolor_->getButton()->click(); }


void uiODFaultToolMan::outputColorChg( CallBacker* cb )
{
    const char* lbltxt = "    New color";

    MultiID mid = getObjSel()->validKey();

    if ( cb )
    {
	newcolor_ = manoutputcolor_->color();
	newcolormid_ = mid;
    }
    else
    {
	manoutputcolor_->setColor( newcolor_ );

	if ( !mid.isEmpty() && mid!=newcolormid_ )
	{
	    IOPar iopar;
	    Color curcolor;
	    EM::EMM().readPars( mid, iopar );
	    if ( iopar.get(sKey::Color,curcolor) )
	    {
		manoutputcolor_->setColor( curcolor );
		lbltxt = "Current color";
	    }
	}
    }

    ioPixmap colorpm( 20, 20 );
    colorpm.fill( manoutputcolor_->color() );
    tbcolorbutton_->setPixmap( colorpm );

    if ( settingsdlg_ )
    {
	NotifyStopper ns( manoutputcolor_->colorChanged ); 
	settingsdlg_->getOutputColor()->setColor( manoutputcolor_->color() );
	settingsdlg_->getOutputColor()->setLblText( lbltxt );
	settingsdlg_->getOutputColor()->colorChanged.trigger();
    }
}


void uiODFaultToolMan::stickCopyCB( CallBacker* )
{ transferSticks( true ); }


void uiODFaultToolMan::stickMoveCB( CallBacker* )
{ transferSticks( false ); }


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


void uiODFaultToolMan::transferSticks( bool copy )
{
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

    mDynamicCastGet( EM::Fault3D*, destf3d, destfault );
    RefMan<EM::EMObject> tmpemobj = EM::FaultStickSet::create(EM::EMM());
    mDynamicCastGet( EM::FaultStickSet*, tmpfss, tmpemobj.ptr() );

    if ( !destfault->isEmpty() )
    {
	const int res = uiMSG().question(
			    "Output object already contains fault stick(s)",
			    "Replace", "Merge" , "Cancel", "Transfer message" );

	if ( res < 0 ) 			// Cancel
	    return;

	if ( res || destf3d )		// Replace or transfer to Fault3D
	{
	    destfault->geometry().selectAllSticks( true );

	    if ( !res )			// Merge
	    {
		destf3d->geometry().copySelectedSticksTo(
			    tmpfss->geometry(), tmpfss->sectionID(0), false );
	    }
	    destfault->geometry().removeSelectedSticks( displayAfterwards() );
	}
    }

    mDynamicCastGet( EM::FaultStickSet*, destfss, destfault );
    if ( !destfss )
    	destfss = tmpfss;

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
	fsstof3d.convert();
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

    destfault->setPreferredColor( manoutputcolor_->color(), true );
    displayUpdate();

    bool saved = false;
    if ( saveAfterwards() )
    {
	PtrMan<Executor> executor = destfault->saver();
	saved = executor->execute();
	if ( !saved )
	    uiMSG().error( "Cannot save output object" );
    }

    UndoEvent* undo = new FaultStickTransferUndoEvent( copy, saved );
    EM::EMM().undo().setUserInteractionEnd( EM::EMM().undo().addEvent(undo) );

    afterTransferUpdate();

    if ( newnrselected )
    {
	uiMSG().message( "Output object could not incorporate ",
			 toString(newnrselected), " of the selected sticks!" );
    }
}


#define mGetDisplayVars( destmid, curid, sceneid ) \
\
    MultiID destmid = getObjSel()->validKey(); \
\
    const int curid = curfltd_ ? curfltd_->id() : \
				 ( curfssd_ ? curfssd_->id() : -1 ); \
\
    const int sceneid = appl_.applMgr().visServer()->getSceneID( curid );
    

void uiODFaultToolMan::displayUpdate()
{
    mGetDisplayVars( destmid, curid, sceneid );
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


bool uiODFaultToolMan::isOutputDisplayed() const
{
    mGetDisplayVars( destmid, curid, sceneid );

    if ( destmid.isEmpty() || curid<0 || sceneid<0 ) 
	return false;

    TypeSet<int> destids;
    appl_.applMgr().visServer()->findObject( destmid, destids );

    for ( int idx=0; idx<destids.size(); idx++ )
    {
	if ( sceneid==appl_.applMgr().visServer()->getSceneID(destids[idx]) )
	    return true;
    }
    return false;
}


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


static int removeSeqNumber( BufferString& objname )
{
    char* lastptr = objname.buf() + objname.size() - 1;
    char* ptr = lastptr;
    while ( ptr>objname.buf() && isdigit(*ptr) )
	ptr--;

    if ( *ptr != '_' )
	return -1;

    *ptr = '\0';
    return ptr==lastptr ? -1 : toInt(ptr+1);
}


void uiODFaultToolMan::afterTransferUpdate()
{
    newcolormid_.setEmpty();
    outputComboChg( 0 );

    if ( !mGetSetting(generateSequelName(),sequelnaming_) )
	return;

    const bool colorrandom = mGetSetting( randomSequelColor(), colorrandom_ );
    newcolor_ = colorrandom ? getRandStdDrawColor() : manoutputcolor_->color(); 

    BufferString objname = getObjSel()->getInput();
    int seqnr = removeSeqNumber( objname );
    if ( seqnr < 0 )
	seqnr = 1;

    BufferString sequelname;
    while ( getObjSel()->existingTyped() )
    {
	sequelname = objname;
	sequelname += "_";
	sequelname += ++seqnr;
	tboutputcombo_->setText( sequelname );
    }
}


void uiODFaultToolMan::settingsToggleCB( CallBacker* )
{ showSettings( toolbar_->isOn(settingsbutidx_) ); }


void uiODFaultToolMan::settingsClosedCB( CallBacker* )
{ toolbar_->turnOn( settingsbutidx_, false ); }


void uiODFaultToolMan::surveyChg( CallBacker* )
{
    manfaultoutput_->getObjSel()->inpBox()->setEmpty();
    manfssoutput_->getObjSel()->inpBox()->setEmpty();

    if ( settingsdlg_ )
    {
	settingsdlg_->setOutputFields( *manfaultoutput_->getObjSel()->inpBox(),
				       *manfssoutput_->getObjSel()->inpBox() );
    }

    tboutputcombo_->setEmpty();
    outputComboChg( 0 );
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
