/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsynthdatamgr.h"
#include "uisynthgenparams.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uisplitter.h"

#include "od_helpids.h"

using SynthSeis::GenParams;



uiStratSynthDataMgr::uiStratSynthDataMgr( uiParent* p, DataMgr& dm )
    : uiDialog(p,uiDialog::Setup(tr("Synthetic Data"),mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
				.modal(false).applybutton(true) )
    , mgr_(dm)
    , applyReq(this)
    , selChg(this)
{
    setForceFinalise( true );
    setCtrlStyle( CloseOnly );

    uiGroup* selgrp = new uiGroup( this, "DataSet List Group" );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Synthetic Data Set"),
			 uiListBox::AboveMid );
    selfld_ = new uiListBox( selgrp, su );
    for ( int idx=0; idx<mgr_.nrSynthetics(); idx++ )
    {
	const auto id = mgr_.getIDByIdx( idx );
	if ( !mgr_.isStratProp(id) )
	    selfld_->addItem( mgr_.nameOf(id) );
    }
    selfld_->setCurrentItem( 0 );

    rmbut_ = new uiPushButton( selgrp, tr("Remove selected"),
		      mCB(this,uiStratSynthDataMgr,rmCB), true );
    rmbut_->setIcon( "remove" );
    rmbut_->attach( centeredBelow, selfld_ );

    uiGroup* rightgrp = new uiGroup( this, "Right Group" );
    rightgrp->setStretch( 1, 1 );

    gpfld_ = new uiSynthGenParams( rightgrp, mgr_ );

    addasnewbut_ = new uiPushButton( rightgrp, tr("Add as new"),
			mCB(this,uiStratSynthDataMgr,addAsNewCB), true );
    addasnewbut_->setIcon( "addnew" );
    addasnewbut_->attach( alignedBelow, gpfld_ );

    uiSplitter* splitter = new uiSplitter( this );
    splitter->addGroup( selgrp );
    splitter->addGroup( rightgrp );

    mAttachCB( postFinalise(), uiStratSynthDataMgr::initWin );
}


uiStratSynthDataMgr::~uiStratSynthDataMgr()
{
    detachAllNotifiers();
}


#define mSelNotif selfld_->selectionChanged

void uiStratSynthDataMgr::initWin( CallBacker* )
{
    updUi();

    mAttachCB( mSelNotif, uiStratSynthDataMgr::selChgCB );
    mAttachCB( gpfld_->nameChanged, uiStratSynthDataMgr::nmChgCB );
}


void uiStratSynthDataMgr::updUi()
{
    const int oldselidx = selfld_->currentItem();
    BufferStringSet nms;
    for ( int idx=0; idx<mgr_.nrSynthetics(); idx++ )
    {
	const GenParams& gp = mgr_.getGenParams( mgr_.getIDByIdx(idx) );
	if ( !gp.isStratProp() )
	    nms.add( gp.name_ );
    }

    int newselidx = oldselidx;
    if ( !nms.validIdx(newselidx) )
	newselidx = nms.size() - 1;
    NotifyStopper ns( mSelNotif );
    selfld_->setEmpty();
    selfld_->addItems( nms );
    if ( newselidx >= 0 )
    {
	selfld_->setCurrentItem( newselidx );
	gpfld_->setByName( nms.get(newselidx) );
    }
    if ( newselidx != oldselidx )
	selChg.trigger();

    prevselidx_ = newselidx;

    updAddNewBut();
    updRmBut();
}


uiStratSynthDataMgr::SynthID uiStratSynthDataMgr::idForIdx( int idx ) const
{
    return idx < 0 ? SynthID() : mgr_.find( selfld_->itemText(idx) );
}


uiStratSynthDataMgr::SynthID uiStratSynthDataMgr::curID() const
{
    return idForIdx( selfld_->currentItem() );
}


const SynthSeis::GenParams& uiStratSynthDataMgr::curGenPars() const
{
     return mgr_.getGenParams( curID() );
}


bool uiStratSynthDataMgr::applyOK()
{
    commitEntry( selfld_->currentItem() );
    applyReq.trigger();
    return mgr_.nrTraces() > 0;
}


void uiStratSynthDataMgr::rmCB( CallBacker* )
{
    mgr_.removeSynthetic( curID() );
    updUi();
}


void uiStratSynthDataMgr::commitEntry( int idx )
{
    if ( idx < 0 )
	return;
    const auto id = idForIdx( idx );
    if ( !id.isValid() )
	{ pErrMsg("Huh"); return; }

    GenParams gp( mgr_.getGenParams(id) );
    gpfld_->get( gp );
    mgr_.setSynthetic( id, gp );
}


void uiStratSynthDataMgr::selChgCB( CallBacker* )
{
    const auto newselidx = selfld_->currentItem();
    if ( newselidx == prevselidx_ )
	return;

    commitEntry( prevselidx_ );
    selfld_->setItemText( prevselidx_, gpfld_->getName() );

    prevselidx_ = newselidx;
    gpfld_->set( curGenPars() );
}


void uiStratSynthDataMgr::nmChgCB( CallBacker* )
{
    updAddNewBut();
}


void uiStratSynthDataMgr::updAddNewBut()
{
    const BufferString nm( gpfld_->getName() );
    const bool isnewandvalid = nm.size() > 1 && !mgr_.find(nm).isValid();
    addasnewbut_->setSensitive( isnewandvalid );
}


void uiStratSynthDataMgr::updRmBut()
{
    rmbut_->setSensitive( selfld_->size() > 1 );
}


void uiStratSynthDataMgr::addAsNewCB( CallBacker* )
{
    GenParams gp;
    gpfld_->get( gp );
    auto newid = mgr_.addSynthetic( gp );
    if ( !newid.isValid() )
	{ pErrMsg("Huh"); return; }

    NotifyStopper ns( mSelNotif );
    selfld_->addItem( gp.name_ );
    prevselidx_ = selfld_->size() - 1;
    selfld_->setCurrentItem( prevselidx_ );
    ns.enableNotification();

    updUi();
}


bool uiStratSynthDataMgr::rejectOK()
{
    applyOK();
    return true;
}
