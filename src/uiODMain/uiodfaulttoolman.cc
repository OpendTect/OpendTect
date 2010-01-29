/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Jaap Glas
 Date: 		December 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodfaulttoolman.cc,v 1.2 2010-01-29 05:38:53 cvsnanne Exp $";


#include "uiodfaulttoolman.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "visdataman.h"
#include "visfaultsticksetdisplay.h"
#include "visselman.h"

#include "uibutton.h"
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
#include "uitoolbar.h"
#include "uivispartserv.h"


uiFaultStickTransferDlg::uiFaultStickTransferDlg( uiParent* p,
						  uiComboBox* tboutputcombo )
    : uiDialog( p, Setup("Faultstick transfer","Transfer settings","0.0.0")
			 .modal(false) )
    , tboutputcombo_( tboutputcombo )
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

    displayfld_ = new uiCheckBox( this, "Display after transfer" );
    displayfld_->attach( alignedBelow, faultoutputfld_ );

    finaliseDone.notify( mCB(this,uiFaultStickTransferDlg,outputTypeChg) );
}


uiFaultStickTransferDlg::~uiFaultStickTransferDlg()
{
    outtypefld_->valuechanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputTypeChg) );
    faultoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );
    fssoutputfld_->getObjSel()->inpBox()->editTextChanged.remove(
			mCB(this,uiFaultStickTransferDlg,outputComboChg) );

    finaliseDone.remove( mCB(this,uiFaultStickTransferDlg,outputTypeChg) );
}


void uiFaultStickTransferDlg::outputTypeChg( CallBacker* )
{
    faultoutputfld_->display( transferToFault() );
    fssoutputfld_->display( !transferToFault() );
    outputComboChg( 0 );
}


void uiFaultStickTransferDlg::outputComboChg( CallBacker* )
{
    if ( !tboutputcombo_ )
	return;

    const uiComboBox* dlgoutputcombo = getDlgOutputCombo(); 
    tboutputcombo_->setEditText( dlgoutputcombo->text() );

    for ( int idx=0; idx<dlgoutputcombo->size(); idx++ )
    {
	const char* itmtxt = dlgoutputcombo->textOfItem(idx);
	if ( tboutputcombo_->indexOf(itmtxt) < 0 )
	   tboutputcombo_->addItem( itmtxt );
    }
}


bool uiFaultStickTransferDlg::transferToFault() const
{
    return outtypefld_->getBoolValue();
}


const uiIOObjSel* uiFaultStickTransferDlg::getObjSel() const
{
    uiIOObjSel* objsel = fssoutputfld_->getObjSel();
    if ( transferToFault() )
	objsel = faultoutputfld_->getObjSel();

    objsel->processInput();
    return objsel;
}


bool uiFaultStickTransferDlg::displayAfterwards() const
{
    return displayfld_->isChecked();
}


uiComboBox* uiFaultStickTransferDlg::getDlgOutputCombo()
{
    if ( transferToFault() )
	return faultoutputfld_->getObjSel()->inpBox();
    else
	return fssoutputfld_->getObjSel()->inpBox();
}


//============================================================================


uiODFaultToolMan::uiODFaultToolMan( uiODMain* appl )
    : appl_(appl)
    , curfssd_(0)
    , curemid_(-1)
{
    toolbar_ = new uiToolBar( appl_, "Fault stick control", uiToolBar::Bottom );
    editselbutidx_ = toolbar_->addButton( "",
	    			mCB(this,uiODFaultToolMan,editSelectToggleCB),
				"Edit/Sel", true );
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

    settingsbutidx_ = toolbar_->addButton( "",
	    			mCB(this,uiODFaultToolMan,popupSettingsCB),
				"Settings", false );

    visBase::DM().selMan().selnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.notify(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.notify( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );

    settingsdlg_ = new uiFaultStickTransferDlg( appl_, tboutputcombo_ );

    appl_->finaliseDone.notify( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
}


uiODFaultToolMan::~uiODFaultToolMan()
{
    delete settingsdlg_;
    delete toolbar_;

    tboutputcombo_->selectionChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );
    tboutputcombo_->editTextChanged.remove(
				mCB(this,uiODFaultToolMan,outputComboChg) );

    visBase::DM().selMan().selnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemSelCB) );
    visBase::DM().selMan().deselnotifier.remove(
				mCB(this,uiODFaultToolMan,treeItemDeselCB) );

    EM::EMM().addRemove.remove( mCB(this,uiODFaultToolMan,addRemoveEMObjCB) );
    appl_->finaliseDone.remove( mCB(this,uiODFaultToolMan,finaliseDoneCB) );
}


void uiODFaultToolMan::finaliseDoneCB( CallBacker* )
{
    clearCurDisplayObj();
}


uiToolBar* uiODFaultToolMan::getToolBar() const
{ return toolbar_; }


void uiODFaultToolMan::treeItemSelCB( CallBacker* cb )
{
    mCBCapsuleUnpack( int, selid, cb );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCast( visSurvey::FaultStickSetDisplay*, curfssd_, dataobj );
    if ( !curfssd_ )
	return;

    curemid_ = curfssd_->getEMID();
    toolbar_->display( true );
    toolbar_->setSensitive( true );
    toolbar_->turnOn( editselbutidx_, curfssd_->isInStickSelectMode() );
    editSelectToggleCB( 0 );
}


void uiODFaultToolMan::treeItemDeselCB( CallBacker* cb )
{
    mCBCapsuleUnpack( int, selid, cb );
    visBase::DataObject* dataobj = visBase::DM().getObject( selid );
    mDynamicCastGet( visSurvey::FaultStickSetDisplay*, oldfss, dataobj );
    if ( oldfss == curfssd_ )
	clearCurDisplayObj();
}


void uiODFaultToolMan::addRemoveEMObjCB( CallBacker* cb )
{
    if ( !EM::EMM().getObject(curemid_) )
    {
	clearCurDisplayObj();
    }
}


void uiODFaultToolMan::clearCurDisplayObj()
{
    curfssd_ = 0;
    curemid_ = -1;
    toolbar_->display( false );
    toolbar_->setSensitive( false );
    settingsdlg_->close();
}


void uiODFaultToolMan::editSelectToggleCB( CallBacker* )
{
    const bool selmode = toolbar_->isOn( editselbutidx_ );
    if ( curfssd_ )
	curfssd_->setStickSelectMode( selmode );

    toolbar_->setSensitive( transferbutidx_,  selmode );
    toolbar_->setSensitive( settingsbutidx_, selmode );
    tboutputcombo_->setSensitive( selmode );

    if ( !selmode )
	settingsdlg_->close();
}


void uiODFaultToolMan::outputComboChg( CallBacker* )
{
    uiComboBox* dlgoutputcombo = settingsdlg_->getDlgOutputCombo();
    dlgoutputcombo->setEditText( tboutputcombo_->text() );
}


void uiODFaultToolMan::stickTransferCB( CallBacker* )
{
    if ( !curfssd_ )
	return;

    EM::EMObject* fromemobj = EM::EMM().getObject( curemid_ );
    mDynamicCastGet( EM::FaultStickSet*, fromfss, fromemobj );
    if ( !fromfss )
	return;

    if ( !fromfss->geometry().nrSelectedSticks() )
    {
	uiMSG().error( "No selected fault stick(s) to transfer" );
	return;
    }

    const uiIOObjSel* objsel = settingsdlg_->getObjSel();
    const MultiID tomid = objsel->key(); 
    EM::EMM().loadIfNotFullyLoaded( tomid );
    EM::ObjectID toemid = EM::EMM().getObjectID( tomid );
    EM::EMObject* toemobj = EM::EMM().getObject( toemid );
    if ( !toemobj )
	return;

    mDynamicCastGet( EM::FaultStickSet*, tofss, toemobj );
    mDynamicCastGet( EM::Fault3D*, toflt, toemobj );

    if ( fromfss == tofss )
	return;

    if ( !toemobj->isEmpty() )
    {
	const int res = uiMSG().question( "Output file already exists!",
			"Overwrite", "Merge" , "Cancel", "Transfer message" );
	if ( res < 0 ) 
	    return;

	if ( res > 0 )
	{
	    if ( tofss )
	    {
		tofss->geometry().selectAllSticks();
		tofss->geometry().removeSelectedSticks();
	    }
	    if ( toflt )
	    {
		toflt->geometry().selectAllSticks();
		toflt->geometry().removeSelectedSticks();
	    }
	}

    }

    mDynamicCastGet( EM::FaultStickSet*, tmpfss,
		     EM::FaultStickSet::create(EM::EMM()) );
    if ( settingsdlg_->transferToFault() )
    	tofss = tmpfss;

    fromfss->geometry().copySelectedSticksTo( tofss->geometry(),
					      tofss->sectionID(0) );
    fromfss->geometry().removeSelectedSticks();
    fromemobj->setChangedFlag();

    if ( settingsdlg_->transferToFault() )
    {
	EM::FSStoFault3DConverter::Setup setup;
	EM::FSStoFault3DConverter fsstof3d( setup, *tofss, *toflt );
	fsstof3d.convert();
    }

    PtrMan<Executor> executor = toemobj->saver();
    if ( !executor->execute() )
    {
	uiMSG().error( "Cannot save output object" );
    }

    if ( settingsdlg_->displayAfterwards() )
    {
	TypeSet<int> todisplayids;
	appl_->applMgr().visServer()->findObject( tomid, todisplayids );
	if ( todisplayids.isEmpty() )
	{
	    const int curfssdisplayid = curfssd_->id();
	    TypeSet<int> sceneids;
	    appl_->applMgr().visServer()->getChildIds( -1, sceneids );
	    if ( !sceneids.isEmpty() )
	    {
		appl_->sceneMgr().addEMItem( toemid, sceneids[0] );
		appl_->sceneMgr().updateTrees();
	    }
	    appl_->applMgr().visServer()->setSelObjectId( curfssdisplayid );
	}
    }

    EM::EMM().removeObject( tmpfss );
}


void uiODFaultToolMan::popupSettingsCB( CallBacker* )
{
    settingsdlg_->go();
}
