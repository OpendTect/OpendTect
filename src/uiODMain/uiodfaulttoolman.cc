/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttoolman.cc,v 1.6 2010-03-16 10:02:46 cvsbert Exp $";


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
    colorfld_->colorChanged.notify(
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


void uiFaultStickTransferDlg::outputTypeChg( CallBacker* )
{
    faultoutputfld_->display( transferToFault() );
    fssoutputfld_->display( !transferToFault() );
    outputComboChg( 0 );
}


void uiFaultStickTransferDlg::outputComboChg( CallBacker* )
{
    outputColorChg( 0 );
    displayChg( 0 );

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

    MultiID mid;
    if ( getObjSel()->existingTyped() )
	mid = getObjSel()->key( true );

    if ( cb )
    {
	newcolor_ = colorfld_->color();
	newcolormid_ = mid;
    }
    else if ( !mid.isEmpty() && mid!=newcolormid_ )
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

    NotifyStopper ns( ftbman_->getOutputColor()->colorChanged );
    ftbman_->getOutputColor()->setColor( colorfld_->color() );
}


void uiFaultStickTransferDlg::displayChg( CallBacker* )
{
    if ( !getObjSel()->existingTyped() )                             
	displayfld_->setSensitive( true );
    else
    {
	const MultiID mid = getObjSel()->key( true );
	TypeSet<int> displayids;
	appl_.applMgr().visServer()->findObject( mid, displayids );
	displayfld_->setSensitive( mid.isEmpty() || displayids.isEmpty() );
    }
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
    newcolormid_.setEmpty();
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
				"Edit/Select", true );
    toolbar_->addSeparator();

    removalbutidx_ = toolbar_->addButton( "trashcan.png",
	    			mCB(this,uiODFaultToolMan,stickRemovalCB),
				"Remove selected sticks", false );

    copybutidx_ = toolbar_->addButton( "copyobj.png",
	    			mCB(this,uiODFaultToolMan,stickCopyCB),
				"Copy selected sticks", false );

    movebutidx_ = toolbar_->addButton( "filelocation.png",
	    			mCB(this,uiODFaultToolMan,stickMoveCB),
				"Move selected sticks", false );
    toolbar_->addSeparator();

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
    tboutputcolor_->colorChanged.notify(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );
    toolbar_->addObject( tboutputcolor_->getButton() );

    settingsbutidx_ = toolbar_->addButton( "tracker-settings.png",
	    			mCB(this,uiODFaultToolMan,settingsToggleCB),
				"Transfer settings", true );
    toolbar_->addSeparator();

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );

    settingsdlg_ = new uiFaultStickTransferDlg( appl_, this );
    settingsdlg_->windowClosed.notify(
				mCB(this,uiODFaultToolMan,settingsClosedCB) );

    appl_.finaliseDone.notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
    deseltimer_.tick.notify( mCB(this,uiODFaultToolMan,deselTimerCB) );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    tboutputcombo_->selectionChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcombo_->editTextChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcolor_->colorChanged.remove(
	    			mCB(this,uiODFaultToolMan,outputColorChg) );

    visBase::DM().selMan().selnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.remove( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    settingsdlg_->windowClosed.remove(
				mCB(this,uiODFaultToolMan,settingsClosedCB) );
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

	const EM::EMObject* emobj = EM::EMM().getObject( curemid_ );
	if ( !emobj || emobj->isEmpty() )
	    toolbar_->turnOn( editselbutidx_, false );

	editSelectToggleCB( 0 );
	enableToolbar( true );
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

	showSettings( toolbar_->isOn(settingsbutidx_) );
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


void uiODFaultToolMan::editSelectToggleCB( CallBacker* )
{
    const bool selmode = toolbar_->isOn( editselbutidx_ );
    if ( curfssd_ )
	curfssd_->setStickSelectMode( selmode );
    if ( curfltd_ )
	curfltd_->setStickSelectMode( selmode );

    toolbar_->setSensitive( removalbutidx_,  selmode );
    toolbar_->setSensitive( copybutidx_, selmode );
    toolbar_->setSensitive( movebutidx_, selmode );
    toolbar_->setSensitive( settingsbutidx_, selmode );
    tboutputcombo_->setSensitive( selmode );
    tboutputcolor_->getButton()->setSensitive( selmode );

    showSettings( selmode && toolbar_->isOn(settingsbutidx_) );

    appl_.applMgr().visServer()->turnSelectionModeOn( selmode );
}


void uiODFaultToolMan::outputComboChg( CallBacker* )
{ settingsdlg_->getOutputCombo()->setText( tboutputcombo_->text() ); }


void uiODFaultToolMan::outputColorChg( CallBacker* )
{
    settingsdlg_->getOutputColor()->setColor( tboutputcolor_->color() );
    settingsdlg_->getOutputColor()->colorChanged.trigger();
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

    const MultiID destmid = settingsdlg_->getObjSel()->key();
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
	const int res = uiMSG().question( "Output file already exists!",
			"Overwrite", "Merge" , "Cancel", "Transfer message" );

	if ( res < 0 ) 			// Cancel
	{
	    EM::EMM().removeObject( tmpfss );
	    return;
	}

	if ( res || destf3d )		// Overwrite or transfer to Fault3D
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

    const int newnrselected = srcfault->geometry().nrSelectedSticks();
    if ( !copy && oldnrselected!=newnrselected )
	srcfault->setChangedFlag();

    destfault->setPreferredColor( tboutputcolor_->color() );
    PtrMan<Executor> executor = destfault->saver();
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
	    appl_.sceneMgr().addEMItem( destemid, sceneids[0] );
	    appl_.sceneMgr().updateTrees();
	    appl_.applMgr().visServer()->setSelObjectId( curdisplayid );
	}
    }

    settingsdlg_->afterTransferUpdate();

    if ( newnrselected )
    {
	uiMSG().message( "Output fault could not fit in ",
			 toString(newnrselected), " of the selected sticks!" );
    }
}


void uiODFaultToolMan::settingsToggleCB( CallBacker* )
{ showSettings( toolbar_->isOn(settingsbutidx_) ); }


void uiODFaultToolMan::settingsClosedCB( CallBacker* )
{ toolbar_->turnOn( settingsbutidx_, false ); }
