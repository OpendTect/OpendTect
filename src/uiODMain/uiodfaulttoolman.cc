/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttoolman.cc,v 1.11 2010-06-18 14:50:26 cvsjaap Exp $";


#include "uiodfaulttoolman.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "pixmap.h"
#include "randcolor.h"
#include "timefun.h"
#include "undo.h"
#include "visdataman.h"
#include "visfaultsticksetdisplay.h"
#include "visfaultdisplay.h"
#include "visselman.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uivispartserv.h"


static void comboCopy( const uiComboBox& from, uiComboBox& to )
{
    NotifyStopper notifystopper( to.editTextChanged );

    to.empty();
    for ( int idx=0; idx<from.size(); idx++ )
	to.addItem( from.textOfItem(idx) );

    to.setText( from.text() );
}


uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiODMain& appl,
						  const Setup& su,
						  uiODFaultToolMan* ftbman )
    : uiDialog( &appl, uiDialog::Setup("Faultstick transfer",
				 "Transfer settings",mTODOHelpID).modal(false) )
    , appl_( appl )
    , ftbman_( ftbman )
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

    displayfld_ = new uiCheckBox( this, "Display after transfer" );
    displayfld_->setChecked( su.displayafter_ );
    displayfld_->attach( alignedBelow, colorfld_ );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, displayfld_ );

    sequelnamefld_ = new uiCheckBox( this,"Create sequel name after transfer" );
    sequelnamefld_->setChecked( su.sequelnaming_ );
    sequelnamefld_->activated.notify(
			mCB(this,uiFaultStickTransferDlg,sequelNameCB) );
    sequelnamefld_->attach( ensureBelow, horsep );
    sequelnamefld_->attach( alignedBelow, displayfld_ );

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
    ftbman_->getOutputCombo()->editTextChanged.trigger();
    displayChg( 0 );
}


void uiFaultStickTransferDlg::outputColorChg( CallBacker* )
{
    NotifyStopper ns( colorfld_->colorChanged );
    ftbman_->getOutputColor()->setColor( colorfld_->color() );
    ftbman_->getOutputColor()->colorChanged.trigger();
}


void uiFaultStickTransferDlg::displayChg( CallBacker* )
{ displayfld_->setSensitive( !ftbman_->isOutputDisplayed() ); }


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


bool uiFaultStickTransferDlg::startDisplayAfter() const
{ return displayfld_->isChecked(); }


bool uiFaultStickTransferDlg::createSequelName() const
{ return sequelnamefld_->isChecked(); }


bool uiFaultStickTransferDlg::randomSequelColor() const
{ return !sequelcolorfld_->getBoolValue(); }


void uiFaultStickTransferDlg::setOutputFields( const uiComboBox& faultcombo,
					       const uiComboBox& fsscombo )
{
    comboCopy( faultcombo, *faultoutputfld_->getObjSel()->inpBox() );
    comboCopy( fsscombo, *fssoutputfld_->getObjSel()->inpBox() );
    displayChg( 0 );
}


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
    editselbutidx_ = toolbar_->addButton( "editsticks.png",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				"Edit/select sticks", true );
    toolbar_->addSeparator();

    removalbutidx_ = toolbar_->addButton( "removesticks.png",
	    			mCB(this,uiODFaultToolMan,stickRemovalCB),
				"Remove selected sticks", false );

    copybutidx_ = toolbar_->addButton( "copysticks.png",
	    			mCB(this,uiODFaultToolMan,stickCopyCB),
				"Copy selected sticks", false );

    movebutidx_ = toolbar_->addButton( "movesticks.png",
	    			mCB(this,uiODFaultToolMan,stickMoveCB),
				"Move selected sticks", false );
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

    tbcolorbutton_ = new uiPushButton( toolbar_, "",
			    mCB(this,uiODFaultToolMan,colorPressedCB), false );
    tbcolorbutton_->setName( "Output color" );
    tbcolorbutton_->setToolTip( tbcolorbutton_->name() );
    toolbar_->addObject( tbcolorbutton_ );

    settingsbutidx_ = toolbar_->addButton( "faulttoolsettings.png",
	    			mCB(this,uiODFaultToolMan,settingsToggleCB),
				"Transfer settings", true );
    toolbar_->addSeparator();

    undobutidx_ = toolbar_->addButton( "undo.png",
	    			mCB(this,uiODFaultToolMan,undoCB),
				"Undo", false );

    redobutidx_ = toolbar_->addButton( "redo.png",
	    			mCB(this,uiODFaultToolMan,redoCB),
				"Redo", false );

    toolbar_->addSeparator();

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );

    appl_.finaliseDone.notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.notify( mCB(this,uiODFaultToolMan,deselTimerCB) );
    IOM().surveyChanged.notify( mCB(this,uiODFaultToolMan,surveyChg) );
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
{ clearCurDisplayObj(); }


uiToolBar* uiODFaultToolMan::getToolBar()
{ return toolbar_; }


uiComboBox* uiODFaultToolMan::getOutputCombo()
{ return tboutputcombo_; }


uiColorInput* uiODFaultToolMan::getOutputColor()
{ return manoutputcolor_; }


void uiODFaultToolMan::treeItemSelCB( CallBacker* cb )
{
    deseltimer_.stop();
    mCBCapsuleUnpack( int, selid, cb );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCast( visSurvey::FaultStickSetDisplay*, curfssd_, dataobj );
    mDynamicCast( visSurvey::FaultDisplay*, curfltd_, dataobj );

    if ( curfssd_ || curfltd_ )
    {
	curemid_ = curfssd_ ? curfssd_->getEMID() : curfltd_->getEMID();

	const EM::EMObject* emobj = EM::EMM().getObject( curemid_ );
	if ( !emobj || emobj->isEmpty() )
	    toolbar_->turnOn( editselbutidx_, false );

	editSelectToggleCB( 0 );
	enableToolbar( true );
	comboCopy( *getObjSel()->inpBox(), *tboutputcombo_ );
	outputColorChg( 0 );
    }
    else
	clearCurDisplayObj();
}


void uiODFaultToolMan::treeItemDeselCB( CallBacker* cb )
{
    mCBCapsuleUnpack( int, selid, cb );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCastGet( visSurvey::FaultStickSetDisplay*, oldfssd, dataobj );
    mDynamicCastGet( visSurvey::FaultDisplay*, oldfltd, dataobj );
    if ( oldfssd==curfssd_ && oldfltd==curfltd_ )
	deseltimer_.start( 100, true );
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

	const bool selmode = toolbar_->isOn( editselbutidx_ );
	showSettings( selmode && toolbar_->isOn(settingsbutidx_) );
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
	    outputColorChg( 0 );
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


void uiODFaultToolMan::editSelectToggleCB( CallBacker* cb )
{
    toolbar_->turnOn( editselbutidx_, true );

    if ( cb )
	selectmode_ = !selectmode_;

    toolbar_->setPixmap( editselbutidx_,
		selectmode_ ? "selectsticks.png" : "editsticks.png" );
    toolbar_->setToolTip( editselbutidx_,
		selectmode_ ? "Switch Select->Edit" : "Switch Edit->Select" );

    if ( curfssd_ )
	curfssd_->setStickSelectMode( selectmode_ );
    if ( curfltd_ )
	curfltd_->setStickSelectMode( selectmode_ );

    updateToolbarCB( 0 );

    showSettings( selectmode_ && toolbar_->isOn(settingsbutidx_) );

    appl_.applMgr().visServer()->turnSelectionModeOn( selectmode_ );
}


void uiODFaultToolMan::updateToolbarCB( CallBacker* )
{
    toolbar_->setSensitive( removalbutidx_, selectmode_ );
    toolbar_->setSensitive( copybutidx_, selectmode_ );
    toolbar_->setSensitive( movebutidx_, selectmode_ );
    toolbar_->setSensitive( settingsbutidx_, selectmode_ );
    tboutputcombo_->setSensitive( selectmode_ );
    tbcolorbutton_->setSensitive( selectmode_ );
    toolbar_->setSensitive( undobutidx_,
			    !selectmode_ && EM::EMM().undo().canUnDo() );
    toolbar_->setSensitive( redobutidx_,
			    !selectmode_ && EM::EMM().undo().canReDo() );
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
    getObjSel()->inpBox()->editTextChanged.trigger();
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

    ioPixmap colorpm( 50, 20 );
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

    srcfault->geometry().removeSelectedSticks();
    srcfault->setChangedFlag();
}


void uiODFaultToolMan::transferSticks( bool copy )
{
    if ( curemid_ < 0 )
	return;

    EM::EMObject* srcemobj = EM::EMM().getObject( curemid_ );
    mDynamicCastGet( EM::Fault*, srcfault, srcemobj );
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
    mDynamicCastGet( EM::Fault*, destfault, EM::EMM().getObject(destemid) );
    if ( !destfault ) 
	return;

    if ( destfault == srcfault )
    {
	uiMSG().error( "No use to transfer selected sticks to myself" );
	return;
    }

    mDynamicCastGet( EM::Fault3D*, destf3d, destfault );
    mDynamicCastGet( EM::FaultStickSet*, tmpfss,
		     EM::FaultStickSet::create(EM::EMM()) );

    if ( !destfault->isEmpty() )
    {
	const int res = uiMSG().question(
			    "Output object already contains fault stick(s)",
			    "Replace", "Merge" , "Cancel", "Transfer message" );

	if ( res < 0 ) 			// Cancel
	{
	    EM::EMM().removeObject( tmpfss );
	    return;
	}

	if ( res || destf3d )		// Replace or transfer to Fault3D
	{
	    destfault->geometry().selectAllSticks( true );

	    if ( !res )			// Merge
	    {
		destf3d->geometry().copySelectedSticksTo( tmpfss->geometry(),
							  tmpfss->sectionID(0));
	    }
	    destfault->geometry().removeSelectedSticks();
	}
    }

    mDynamicCastGet( EM::FaultStickSet*, destfss, destfault );
    if ( !destfss )
    	destfss = tmpfss;

    if ( copy )
	srcfault->geometry().selectStickDoubles( false, &destfss->geometry() );
    else
	srcfault->geometry().removeSelectedDoubles( &destfss->geometry() );

    srcfault->geometry().copySelectedSticksTo( destfss->geometry(),
					       destfss->sectionID(0) );
    if ( destf3d )
    {
	EM::FSStoFault3DConverter::Setup setup;
	if ( destf3d->isEmpty() )
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Auto;
	else if ( destf3d->geometry().areSticksVertical(destf3d->sectionID(0)) )
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Vertical;
	else 
	    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Horizontal;

	EM::FSStoFault3DConverter fsstof3d( setup, *tmpfss, *destf3d );
	fsstof3d.convert();
    }
    EM::EMM().removeObject( tmpfss );

    if ( copy )
	srcfault->geometry().selectStickDoubles( false, &destfault->geometry());
    else
	srcfault->geometry().removeSelectedDoubles( &destfault->geometry() );

    if ( curfssd_ )
	curfssd_->updateKnotMarkers();
    if ( curfltd_ )
	curfltd_->updateKnotMarkers();

    const int newnrselected = srcfault->geometry().nrSelectedSticks();
    if ( !copy && oldnrselected!=newnrselected )
	srcfault->setChangedFlag();

    destfault->setPreferredColor( manoutputcolor_->color() );

    displayUpdate();
    if ( !isOutputDisplayed() )
    {
	PtrMan<Executor> executor = destfault->saver();
	if ( !executor->execute() )
	{
	    uiMSG().error( "Cannot save output object" );
	    return;
	}
    }

    afterTransferUpdate();

    if ( newnrselected )
    {
	uiMSG().message( "Output fault could not incorporate ",
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

    if ( mGetSetting(startDisplayAfter(),displayafter_) )
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


static int removeSeqNumber( BufferString& objname )
{
    char* lastptr = objname.buf() + objname.size() - 1;
    char* ptr = lastptr;
    while ( ptr>objname.buf() && isdigit(*ptr) )
	ptr--;

    if ( *ptr != '_' )
	return -1;

    *ptr = '\0';
    return ptr==lastptr ? -1 : atoi(ptr+1);
}


void uiODFaultToolMan::afterTransferUpdate()
{
    newcolormid_.setEmpty();
    outputComboChg( 0 );

    if ( !mGetSetting(createSequelName(),sequelnaming_) )
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
    manfaultoutput_->getObjSel()->inpBox()->empty();
    manfssoutput_->getObjSel()->inpBox()->empty();

    if ( settingsdlg_ )
    {
	settingsdlg_->setOutputFields( *manfaultoutput_->getObjSel()->inpBox(),
				       *manfssoutput_->getObjSel()->inpBox() );
    }

    tboutputcombo_->empty();
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
