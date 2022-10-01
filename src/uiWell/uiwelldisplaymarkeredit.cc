/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelldisplaymarkeredit.h"

#include "uiwelldisplaycontrol.h"
#include "uicolor.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimenuhandler.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitoolbutton.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "randcolor.h"
#include "sorting.h"
#include "stratlevel.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldata.h"
#include "od_helpids.h"

#define mErrRet(msg,act) { uiMSG().error( msg ); act; }
uiAddEditMrkrDlg::uiAddEditMrkrDlg( uiParent* p, Well::Marker& mrk, bool edit )
    : uiDialog(p,uiDialog::Setup(edit ? tr("Edit Marker"): tr("Add Marker"),
					mNoDlgTitle,mNoHelpKey))
    , marker_(mrk)
{
    namefld_ = new uiGenInput( this, uiStrings::sName(),
                               StringInpSpec("Marker") );

    uiColorInput::Setup csu( mrk.color() );
    csu.lbltxt( uiStrings::sColor() ).withdesc(false);
    colorfld_ = new uiColorInput( this, csu, "Color" );
    colorfld_->attach( alignedBelow, namefld_ );

    stratmrkfld_ = new uiCheckBox( this, tr("Set as regional marker") );
    stratmrkfld_->attach( rightOf, colorfld_ );
    stratmrkfld_->setChecked( true );

    putToScreen();
}


uiAddEditMrkrDlg::~uiAddEditMrkrDlg()
{}


bool uiAddEditMrkrDlg::acceptOK( CallBacker* )
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
	mErrRet( tr("Please specify a marker name"), return false );

    marker_.setName( nm );
    marker_.setColor( colorfld_->color() );
    if ( stratmrkfld_->isChecked() )
    {
	Strat::LevelID lvlid = Strat::LVLS().getIDByName( nm.buf() );
	if ( !lvlid.isValid() )
	    lvlid = Strat::eLVLS().add( nm.buf(), colorfld_->color() );

	marker_.setLevelID( lvlid );
    }

    return true;
}


void uiAddEditMrkrDlg::putToScreen()
{
    namefld_->setText( marker_.name() );
    colorfld_->setColor( marker_.color() );
    stratmrkfld_->setChecked( marker_.levelID().isValid() );
}



uiDispEditMarkerDlg::uiDispEditMarkerDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Edit Markers Dialog"),
				 mNoDlgTitle, mODHelpKey(
				     mDispEditMarkerDlgHelpID) )
				.modal(false))
    , curmrk_(0)
    , hasedited_(false)
    , needsave_(false)
    , ispicking_(true)
    , pickmodechanged(this)
{
    setOkText( tr("OK/Save") );

    mrklist_ = new uiListBox( this, "Markers" );
    mrklist_->rightButtonClicked.notify(
			    mCB(this,uiDispEditMarkerDlg,listRClickCB) );
    mrklist_->setStretch( 2, 2 );

    toolgrp_ = new uiGroup( this, "Tools" );
    toolgrp_->attach( rightOf, mrklist_ );
    CallBack butcb( mCB(this,uiDispEditMarkerDlg,buttonPushedCB) );
    pickbut_ = new uiToolButton( toolgrp_, "seedpickmode",
	tr("Pick marker on display"), mCB(this,uiDispEditMarkerDlg,modeChg) );
    pickbut_->setToggleButton( true );
    pickbut_->setOn( true );

    uiSeparator* modesep = new uiSeparator( toolgrp_, "Mode Sep" );
    modesep->attach( stretchedBelow, pickbut_ );

    addbut_ = new uiToolButton( toolgrp_, "plus", tr("Add Marker"), butcb );
    addbut_->attach( ensureBelow, modesep );
    addbut_->attach( alignedBelow, pickbut_ );
    editbut_ = new uiToolButton( toolgrp_, "edit", tr("Edit Marker"), butcb );
    editbut_->attach( alignedBelow, addbut_ );
    rembut_ = new uiToolButton(toolgrp_, "trashcan", tr("Remove Marker"),butcb);
    rembut_->attach( alignedBelow, editbut_ );
}


uiDispEditMarkerDlg::~uiDispEditMarkerDlg()
{
    deepErase( tmplist_ );
    deepErase( orgmarkerssets_ );
}


void uiDispEditMarkerDlg::modeChg( CallBacker* )
{
    if ( ispicking_ != pickbut_->isOn() )
    {
	ispicking_ = pickbut_->isOn();
	mrklist_->setMultiChoice( false );
	pickmodechanged.trigger();
    }
}


void uiDispEditMarkerDlg::addMarkerSet( Well::MarkerSet& mrks )
{
    Well::MarkerSet* orgmrks = new Well::MarkerSet( mrks );
    orgmarkerssets_ += orgmrks;
    markerssets_ += &mrks;
    fillMarkerList( 0 );
}


void uiDispEditMarkerDlg::addMoveMarker( int iset, float dah, const char* nm )
{
    const bool ispresent = markerssets_[iset]->isPresent( nm );

    Well::Marker* mrk = 0;
    Well::MarkerSet& markers = *markerssets_[iset];
    if ( ispresent )
    {
	mrk = markers.getByName( nm  );
	mrk->setDah( dah );
    }
    else
    {
	ObjectSet<Well::Marker> mrks;
	getMarkerFromAll( mrks, nm );
	if ( mrks.isEmpty() )
	    mErrRet( tr("No marker found"), return );

	mrk = new Well::Marker( *mrks[0] );
	mrk->setDah( dah );
	markers.insertNew( mrk );
    }

    for ( int idx=tmplist_.size()-1; idx>=0; idx-- )
    {
	if ( tmplist_[idx]->name() == nm )
	    delete tmplist_.removeSingle( idx );
    }
}


void uiDispEditMarkerDlg::removeMarker( int idset, const char* nm )
{
    Well::MarkerSet& mrkset = *markerssets_[idset];
    if ( mrkset.isPresent( nm ) )
	delete mrkset.removeSingle( mrkset.indexOf( nm ),true );
}


bool uiDispEditMarkerDlg::acceptOK( CallBacker* )
{
    needsave_ = hasedited_;
    hasedited_ = false;
    return true;
}


bool uiDispEditMarkerDlg::rejectOK( CallBacker* )
{
    needsave_ = false;
    hasedited_ = false;
    return true;
}


void uiDispEditMarkerDlg::buttonPushedCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb);
    if ( tb == addbut_ )
	addNewMrkrList();
    else if ( tb == editbut_ )
	editMrkrList();
    else if ( tb == rembut_ )
	removeMrkrFromList();
}


void uiDispEditMarkerDlg::addNewMrkrList()
{
    Well::Marker* mrk = new Well::Marker( "Name", 0 );
    mrk->setColor( OD::getRandStdDrawColor() );
    uiAddEditMrkrDlg dlg( this, *mrk, false );
    if ( dlg.go() )
    {
	tmplist_ += mrk;
	fillMarkerList(0);
	if ( mrklist_->isPresent( mrk->name() ) )
	    mrklist_->setCurrentItem( mrk->name().buf() );
    }
    else
	delete mrk;
}


void uiDispEditMarkerDlg::editMrkrList()
{
    const int selidx = mrklist_->currentItem();
    if ( selidx < 0 )
	return;

    BufferString mrknm = mrklist_->getText();
    ObjectSet<Well::Marker> mrks;
    getMarkerFromAll( mrks, mrknm );
    if ( mrks.isEmpty() )
	mErrRet( tr("No marker found"), return );

    Well::Marker* mrk = new Well::Marker( *mrks[0] );
    uiAddEditMrkrDlg dlg( this, *mrk, true );
    if ( !dlg.go() )
	return;

    mrks.erase();
    getMarkerFromAll( mrks, mrknm );
    if ( mrks.isEmpty() )
	{ delete mrk; return; }

    for ( int idx=0; idx<mrks.size(); idx++ )
    {
	Well::Marker* edmrk = mrks[idx];
	edmrk->setName( mrk->name() );
	edmrk->setColor( mrk->color() );
	edmrk->setLevelID( mrk->levelID() );
    }

    mrklist_->setCurrentItem( mrk->name().buf() );
    hasedited_ = true;

    delete mrk;
}


void uiDispEditMarkerDlg::getMarkerFromAll( ObjectSet<Well::Marker>& mrks,
						  const char* mrknm )
{
    for ( int idwd=0; idwd<markerssets_.size(); idwd++ )
    {
	Well::MarkerSet& mrkset = *markerssets_[idwd];;
	Well::Marker* mrk = mrkset.getByName( mrknm );
	if ( mrk )
	    mrks += mrk;
    }
    Well::Marker* tmpmrk = getMarkerFromTmpList( mrknm );
    if ( tmpmrk ) mrks += tmpmrk;
}


bool uiDispEditMarkerDlg::removeMrkrFromList()
{
    BufferString mrknm = mrklist_->getText();
    for ( int idx=tmplist_.size()-1; idx>=0; idx-- )
    {
	if ( tmplist_[idx]->name() == mrknm )
	{
	    delete tmplist_.removeSingle( idx );
	    fillMarkerList(0);
	    return true;
	}
    }

    uiString msg = tr("This will remove %1 from all the wells."
		      "\n\nDo you want to continue ? ").arg(mrknm);

    if ( uiMSG().askContinue( msg ) )
    {
	for ( int idx=0; idx<markerssets_.size(); idx++ )
	{
	    Well::MarkerSet& mrkset = *markerssets_[idx];
	    if ( mrkset.isPresent( mrknm ) )
	    {
		delete mrkset.removeSingle( mrkset.indexOf( mrknm ),true );
	    }
	}
	hasedited_ = true;
	return true;
    }
    return false;
}


void uiDispEditMarkerDlg::listRClickCB( CallBacker* )
{
    uiMenu mnu( this, uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(tr("Add New"))), 0 );
    if ( !mrklist_->isEmpty() )
    {
	mnu.insertAction(
	    new uiAction(m3Dots(uiStrings::sEdit())), 1 );
	mnu.insertAction(
	    new uiAction(m3Dots(uiStrings::sRemove())), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid < 0 )
	return;
    else if ( mnuid == 0 )
    {
	addNewMrkrList();
    }
    else if ( mnuid == 1 )
    {
	editMrkrList();
    }
    else if ( mnuid ==2 )
    {
	removeMrkrFromList();
    }
}


void uiDispEditMarkerDlg::fillMarkerList( CallBacker* )
{
    const BufferString selnm = mrklist_->nrChosen() ? mrklist_->getText()
						    : sKey::EmptyString().buf();
    if ( mrklist_->size() ) mrklist_->setEmpty();
    BufferStringSet mrknms; TypeSet<OD::Color> mrkcols; TypeSet<float> dahs;

#define mAddMrkToList(mrk)\
if ( mrknms.addIfNew( mrk.name() ) )\
{\
    mrkcols += mrk.color();\
    dahs += mrk.dah();\
}

    for ( int idwd=0; idwd<markerssets_.size(); idwd++ )
    {
	const Well::MarkerSet& mrkset = *markerssets_[idwd];
	for ( int idmrk=0; idmrk<mrkset.size(); idmrk++ )
	{
	    const Well::Marker& mrk = *mrkset[idmrk];
	    mAddMrkToList( mrk )
	}
    }
    for ( int idmrk=0; idmrk<tmplist_.size(); idmrk++ )
    {
	const Well::Marker& mrk = *tmplist_[idmrk];
	mAddMrkToList( mrk )
    }
    TypeSet<int> idxs;
    for ( int idx=0; idx<dahs.size(); idx++)
	idxs += idx;
    sort_coupled( dahs.arr(), idxs.arr(), dahs.size() );

    for ( int idx=0; idx<mrknms.size(); idx++ )
    {
	mrklist_->addItem( toUiString(mrknms.get( idxs[idx]) ),
							  mrkcols[idxs[idx]] );
	colors_ += mrkcols[idxs[idx]];
    }

    if ( mrklist_->isEmpty() )
    {
	Well::Marker* mrk = new Well::Marker( "Name", 0 );
	mrk->setColor( OD::getRandStdDrawColor() );
	tmplist_ += mrk;
    }

    if ( isPicking() )
    {
	const int selidx = mrklist_->indexOf( selnm );
	if ( mrklist_->validIdx(selidx) )
	    mrklist_->setCurrentItem( selidx );
    }
}


Well::Marker* uiDispEditMarkerDlg::getMarkerFromTmpList( const char* mrknm )
{
    for ( int idx=0; idx<tmplist_.size(); idx++ )
    {
	if ( tmplist_[idx]->name() == mrknm )
	    return tmplist_[idx];
    }
    return 0;
}



uiWellDispCtrlEditMarkerDlg::uiWellDispCtrlEditMarkerDlg( uiParent* p )
    : uiDispEditMarkerDlg(p)
    , curctrl_(0)
    , curwd_(0)
{
}


uiWellDispCtrlEditMarkerDlg::~uiWellDispCtrlEditMarkerDlg()
{
    detachAllNotifiers();
}


void uiWellDispCtrlEditMarkerDlg::addWellCtrl( uiWellDisplayControl& ctrl,
					      Well::Data& wd  )
{
    ctrls_ += &ctrl;
    wds_ += &wd;
    addMarkerSet( wd.markers() );

    mAttachCB( ctrl.posChanged,uiWellDispCtrlEditMarkerDlg::handleCtrlChangeCB);
    mAttachCB( ctrl.mousePressed,uiWellDispCtrlEditMarkerDlg::handleUsrClickCB);
    mAttachCB( wd.markerschanged, uiWellDispCtrlEditMarkerDlg::fillMarkerList );
}


void uiWellDispCtrlEditMarkerDlg::triggerWDsMarkerChanged()
{
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wds_[idx]->markerschanged.trigger();
    }
}


void uiWellDispCtrlEditMarkerDlg::handleCtrlChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(BufferString,mesg,caller,cb);
    mDynamicCastGet(uiWellDisplayControl*,ctrl,caller)
    curctrl_ = 0; curmrk_ = 0; curwd_ = 0;
    if ( !ctrl || !ctrl->mouseEventHandler()
		|| !ctrl->mouseEventHandler()->hasEvent() ) return;
    curctrl_ = ctrl;
    int widx = ctrls_.indexOf( ctrl );
    curwd_ = ( widx >=0 && widx<wds_.size() ) ? wds_[widx] : 0;
    if ( curctrl_ && curwd_ )
    {
	ObjectSet<Well::Marker>& mrkset = curwd_->markers();
	int curmrkidx = mrkset.indexOf( curctrl_->selMarker() );
	if ( curmrkidx >=0 )
	    curmrk_ = mrkset[curmrkidx];
    }
}


void uiWellDispCtrlEditMarkerDlg::handleUsrClickCB( CallBacker* )
{
    const int idset = wds_.indexOf( curwd_ );
    if ( !curctrl_ || !curwd_ || idset < 0 ) return;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() ) return;

    bool isremove = curctrl_->isCtrlPressed();
    const float dah = curctrl_->dah();
    const char* mrknm = mrklist_->getText();

    if ( !isremove )
    {
	addMoveMarker( idset, dah, mrknm );
	Well::DisplayProperties& disp2d = curwd_->displayProperties(true);
	Well::DisplayProperties& disp3d = curwd_->displayProperties(false);
	ObjectSet<Well::DisplayProperties> dispprops( &disp2d, &disp3d );
	for ( auto* dispprop : dispprops )
	{
	    BufferStringSet markernms =
				dispprop->getMarkers().markerNms( false );
	    markernms.remove( mrknm );
	    dispprop->setMarkerNames( markernms, false );
	}
    }
    else if ( curmrk_ )
    {
	removeMarker( idset, curmrk_->name() );
    }

    mevh->setHandled( true);
    hasedited_ = true;
    curwd_->markerschanged.trigger();
}


bool uiWellDispCtrlEditMarkerDlg::acceptOK( CallBacker* )
{
    needsave_ = hasedited_;
    if ( hasedited_ )
    {
	triggerWDsMarkerChanged();
	hasedited_ = false;
    }

    return true;
}


bool uiWellDispCtrlEditMarkerDlg::rejectOK( CallBacker* )
{
    needsave_ = false;
    if ( hasedited_ )
	askForSavingEditedChanges();
    return true;
}


void uiWellDispCtrlEditMarkerDlg::askForSavingEditedChanges()
{
    if ( !hasedited_ ) return;

    uiString msg = tr("Some markers have been edited."
		      "\n\nDo you want to save those changes? ");
    if ( uiMSG().askGoOn( msg ) )
	needsave_ = true;
    else
    {
	for ( int idx=0; idx<markerssets_.size(); idx++ )
	    *markerssets_[idx] = *orgmarkerssets_[idx];
    }

    triggerWDsMarkerChanged();
    hasedited_ = false;
}


void uiWellDispCtrlEditMarkerDlg::editMrkrList()
{
    uiDispEditMarkerDlg::editMrkrList();

    triggerWDsMarkerChanged();
}


bool uiWellDispCtrlEditMarkerDlg::removeMrkrFromList()
{
    if ( uiDispEditMarkerDlg::removeMrkrFromList() )
    {
	triggerWDsMarkerChanged();
	return true;
    }
    return false;
}
