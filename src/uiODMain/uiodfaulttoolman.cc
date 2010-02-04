/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttoolman.cc,v 1.4 2010-02-04 17:20:24 cvsjaap Exp $";


#include "uiodfaulttoolman.h"

#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "randcolor.h"
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

#include "timefun.h"

uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiODMain& appl,
						  uiODFaultToolMan* ftbman )
    : uiDialog( &appl, Setup("Faultstick transfer","Transfer settings","0.0.0")
		       .modal(false) )
    , appl_( appl )
    , ftbman_( ftbman )
    , newcolor_( getRandStdDrawColor() )
    , newcolormid_( -1 )
{
    setCtrlStyle( LeaveOnly );
    outtypefld_ = new uiGenInput( this, "Output type",
				  BoolInpSpec(true,"Fault","FaultStickSet") );

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
		    uiColorInput::Setup(newcolor_).lbltxt("Current color"),
				  "Output color" );
    colorfld_->colorchanged.notify(
			mCB(this,uiFaultStickTransferDlg,outputColorChg) );
    colorfld_->attach( alignedBelow, faultoutputfld_ );

    displayfld_ = new uiCheckBox( this, "Display after transfer" );
    displayfld_->attach( alignedBelow, colorfld_ );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, displayfld_ );

    sequelnamefld_ = new uiCheckBox( this,
				     "Create sequel name after transfer" );
    sequelnamefld_->setChecked( true );
    sequelnamefld_->activated.notify(
			mCB(this,uiFaultStickTransferDlg,sequelNameCB) );
    sequelnamefld_->attach( ensureBelow, horsep );
    sequelnamefld_->attach( alignedBelow, displayfld_ );

    sequelcolorfld_ = new uiGenInput( this, "Sequel color",
				      BoolInpSpec(true,"Unchanged","Random") );
    sequelcolorfld_->attach( alignedBelow, sequelnamefld_ );

    finaliseDone.notify( mCB(this,uiFaultStickTransferDlg,finaliseDoneCB) );
}


uiFaultStickTransferDlg::~uiFaultStickTransferDlg()
{
    outtypefld_->valuechanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputTypeChg) );
    faultoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );
    fssoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );
    colorfld_->colorchanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputColorChg) );
    sequelnamefld_->activated.remove(
			mCB(this,uiFaultStickTransferDlg,sequelNameCB) );

    finaliseDone.remove( mCB(this,uiFaultStickTransferDlg,finaliseDoneCB) );
}


void uiFaultStickTransferDlg::finaliseDoneCB( CallBacker* )
{
    outputTypeChg( 0 );
    sequelNameCB( 0 ); 
}


void uiFaultStickTransferDlg::outputTypeChg( CallBacker* )
{
    faultoutputfld_->display( transferToFault() );
    fssoutputfld_->display( !transferToFault() );
    outputComboChg( 0 );
}


void uiFaultStickTransferDlg::outputComboChg( CallBacker* )
{
    outputColorChg( 0 );

    if ( !getObjSel()->existingTyped() )
	displayfld_->setSensitive( true );
    else
    {
	const MultiID mid = getObjSel()->key( true );
	TypeSet<int> displayids;
	appl_.applMgr().visServer()->findObject( mid, displayids );
	displayfld_->setSensitive( displayids.isEmpty() );
    }

    if ( !ftbman_ )
	return;

    uiComboBox* dlgoutputcombo = getOutputCombo(); 
    NotifyStopper ns( ftbman_->getOutputCombo()->editTextChanged );
    ftbman_->getOutputCombo()->empty();

    for ( int idx=0; idx<dlgoutputcombo->size(); idx++ )
	ftbman_->getOutputCombo()->addItem( dlgoutputcombo->textOfItem(idx) );

    ftbman_->getOutputCombo()->setText( dlgoutputcombo->text() );
}


void uiFaultStickTransferDlg::outputColorChg( CallBacker* cb )
{
    colorfld_->setLblText( "    New color" );

    MultiID mid( -1 );
    if ( getObjSel()->existingTyped() )
	mid = getObjSel()->key( true );

    if ( cb )
    {
	newcolor_ = colorfld_->color();
	newcolormid_ = mid;
    }
    else if ( mid!=MultiID(-1) && mid!=newcolormid_ )
    {
	IOPar iopar;
	Color curcolor;
	EM::EMM().readPars( mid, iopar );
	iopar.get(sKey::Color,curcolor);
	colorfld_->setColor( curcolor );

	colorfld_->setLblText( "Current color" );
    }
    else
	colorfld_->setColor( newcolor_ );

    if ( !ftbman_ )
	return;

    NotifyStopper ns( ftbman_->getOutputColor()->colorchanged );
    ftbman_->getOutputColor()->setColor( colorfld_->color() );
}


void uiFaultStickTransferDlg::sequelNameCB( CallBacker* cb )
{ sequelcolorfld_->setSensitive( sequelnamefld_->isChecked() ); }


bool uiFaultStickTransferDlg::transferToFault() const
{
    return outtypefld_->getBoolValue();
}


uiIOObjSel* uiFaultStickTransferDlg::getObjSel()
{
    uiIOObjSel* objsel = fssoutputfld_->getObjSel();
    if ( transferToFault() )
	objsel = faultoutputfld_->getObjSel();

    return objsel;
}


bool uiFaultStickTransferDlg::startDisplayingAfterwards() const
{
    return displayfld_->sensitive() && displayfld_->isChecked();
}


uiComboBox* uiFaultStickTransferDlg::getOutputCombo()
{
    if ( transferToFault() )
	return faultoutputfld_->getObjSel()->inpBox();
    else
	return fssoutputfld_->getObjSel()->inpBox();
}


uiColorInput* uiFaultStickTransferDlg::getOutputColor()
{ return colorfld_; }


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


bool uiFaultStickTransferDlg::afterTransferUpdate()
{
    newcolormid_ = MultiID( -1 );
    outputComboChg( 0 );

    if ( !sequelnamefld_->isChecked() )
	return false;

    const bool unchanged = sequelcolorfld_->getBoolValue();
    newcolor_ = unchanged ? colorfld_->color() : getRandStdDrawColor(); 

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
	getObjSel()->setInputText( sequelname );
    }

    return true;
}


//============================================================================


uiODFaultToolMan::uiODFaultToolMan( uiODMain& appl )
    : appl_( appl )
    , tracktbwashidden_( false )
{
    toolbar_ = new uiToolBar( &appl_, "Fault stick control", uiToolBar::Bottom);
    editselbutidx_ = toolbar_->addButton( "",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				"Edit/Sel", true );

    removalbutidx_ = toolbar_->addButton( "trashcan.png",
	    			mCB(this,uiODFaultToolMan,stickRemovalCB),
				"Remove selected sticks", false );
    toolbar_->addSeparator();

    transferbutidx_ = toolbar_->addButton( "",
	    			mCB(this,uiODFaultToolMan,stickTransferCB),
				"Transfer", false );

    tboutputcombo_ = new uiComboBox( toolbar_, "Output name" );
    tboutputcombo_->setReadOnly( false );
    tboutputcombo_->setMinimumWidth( 150 );
    tboutputcombo_->selectionChanged.notify(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcombo_->editTextChanged.notify(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    toolbar_->addObject( tboutputcombo_ );

    tboutputcolor_ = new uiColorInput( 0, uiColorInput::Setup(Color(0,0,0)),
				       "Output color" );
    tboutputcolor_->colorchanged.notify(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );
    toolbar_->addObject( tboutputcolor_->getButton() );

    settingsbutidx_ = toolbar_->addButton( "tracker-settings.png",
	    			mCB(this,uiODFaultToolMan,popupSettingsCB),
				"Transfer settings", false );
    toolbar_->addSeparator();

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );

    settingsdlg_ = new uiFaultStickTransferDlg( appl_, this );

    appl_.finaliseDone.notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.notify( mCB(this,uiODFaultToolMan,deselTimerCB) );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    tboutputcombo_->selectionChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcombo_->editTextChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcolor_->colorchanged.remove(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );

    visBase::DM().selMan().selnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.remove( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    appl_.finaliseDone.remove( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.remove( mCB(this,uiODFaultToolMan,deselTimerCB) );

    delete settingsdlg_;
    delete toolbar_;
    delete tboutputcolor_;
}


void uiODFaultToolMan::finaliseDoneCB( CallBacker* )
{
    settingsdlg_->go();
    settingsdlg_->close();
    clearCurDisplayObj();
}


uiToolBar* uiODFaultToolMan::getToolBar()
{ return toolbar_; }


uiComboBox* uiODFaultToolMan::getOutputCombo()
{ return tboutputcombo_; }


uiColorInput* uiODFaultToolMan::getOutputColor()
{ return tboutputcolor_; }


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
	enableToolbar( true );
	editSelectToggleCB( 0 );
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

	if ( !settingswerehidden_ )
	    settingsdlg_->show();
    }

    toolbar_->display( yn );
    toolbar_->setSensitive( yn );

    if ( !yn )
    {
	settingswerehidden_ = settingsdlg_->isHidden();
	settingsdlg_->close();

	if ( tracktb_->isHidden() )
	    tracktb_->display( !tracktbwashidden_);
    }
}


void uiODFaultToolMan::editSelectToggleCB( CallBacker* )
{
    const bool selmode = toolbar_->isOn( editselbutidx_ );
    if ( curfssd_ )
	curfssd_->setStickSelectMode( selmode );
    if ( curfltd_ )
	curfltd_->setStickSelectMode( selmode );

    toolbar_->setSensitive( removalbutidx_,  selmode );
    toolbar_->setSensitive( transferbutidx_,  selmode );
    toolbar_->setSensitive( settingsbutidx_, selmode );
    tboutputcombo_->setSensitive( selmode );
    tboutputcolor_->getButton()->setSensitive( selmode );

    if ( !selmode )
	settingsdlg_->close();
}


void uiODFaultToolMan::outputComboChg( CallBacker* )
{ settingsdlg_->getOutputCombo()->setText( tboutputcombo_->text() ); }


void uiODFaultToolMan::outputColorChg( CallBacker* )
{
    settingsdlg_->getOutputColor()->setColor( tboutputcolor_->color() );
    settingsdlg_->getOutputColor()->colorchanged.trigger();
}


void uiODFaultToolMan::stickRemovalCB( CallBacker* )
{ transferSticks( true ); }


void uiODFaultToolMan::stickTransferCB( CallBacker* )
{ transferSticks( false ); }


void uiODFaultToolMan::transferSticks( bool removeonly )
{
    if ( curemid_ < 0 )
	return;

    EM::EMObject* fromemobj = EM::EMM().getObject( curemid_ );
    mDynamicCastGet( EM::Fault*, fromfault, fromemobj );
    if ( !fromfault )
	return;

    if ( !fromfault->geometry().nrSelectedSticks() )
    {
	uiMSG().error( "No selected fault stick(s) to transfer" );
	return;
    }

    mDynamicCastGet( EM::FaultStickSet*, tmpfss,
		     EM::FaultStickSet::create(EM::EMM()) );

    EM::ObjectID toemid = tmpfss->id();
    if ( !removeonly )
    {
	const MultiID tomid = settingsdlg_->getObjSel()->key();
	EM::EMM().loadIfNotFullyLoaded( tomid );
	toemid = EM::EMM().getObjectID( tomid );
    }

    mDynamicCastGet( EM::Fault*, tofault, EM::EMM().getObject(toemid) );
    if ( !tofault || tofault==fromfault )
	return;

    if ( !tofault->isEmpty() )
    {
	const int res = uiMSG().question( "Output file already exists!",
			"Overwrite", "Merge" , "Cancel", "Transfer message" );
	if ( res < 0 ) 
	    return;

	if ( res > 0 )
	{
	    tofault->geometry().selectAllSticks();
	    tofault->geometry().removeSelectedSticks();
	}

    }

    mDynamicCastGet( EM::FaultStickSet*, tofss, tofault );
    if ( !tofss )
    	tofss = tmpfss;

    fromfault->geometry().copySelectedSticksTo( tofss->geometry(),
						tofss->sectionID(0) );
    fromfault->geometry().removeSelectedSticks();
    fromfault->setChangedFlag();

    if ( removeonly )
    {
	EM::EMM().removeObject( tmpfss );
	return;
    }

    mDynamicCastGet( EM::Fault3D*, tof3d, tofault );
    if ( tof3d )
    {
	EM::FSStoFault3DConverter::Setup setup;
	EM::FSStoFault3DConverter fsstof3d( setup, *tofss, *tof3d );
	fsstof3d.convert();
    }

    EM::EMM().removeObject( tmpfss );

    tofault->setPreferredColor( tboutputcolor_->color() );
    PtrMan<Executor> executor = tofault->saver();
    if ( !executor->execute() )
    {
	uiMSG().error( "Cannot save output object" );
	return;
    }

    if ( settingsdlg_->startDisplayingAfterwards() )
    {
	const int curdisplayid = curfltd_ ? curfltd_->id() : curfssd_->id();
	TypeSet<int> sceneids;
	appl_.applMgr().visServer()->getChildIds( -1, sceneids );
	if ( !sceneids.isEmpty() )
	{
	    appl_.sceneMgr().addEMItem( toemid, sceneids[0] );
	    appl_.sceneMgr().updateTrees();
	    appl_.applMgr().visServer()->setSelObjectId( curdisplayid );
	}
    }

    settingsdlg_->afterTransferUpdate();
}


void uiODFaultToolMan::popupSettingsCB( CallBacker* )
{
    if ( settingsdlg_->isHidden() )
	settingsdlg_->go();
}
