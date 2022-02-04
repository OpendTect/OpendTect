/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitabstack.h"

#include "ioman.h"
#include "keystrs.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellman.h"
#include "wellmarker.h"

#include "hiddenparam.h"

HiddenParam<uiWellDispPropDlg,Color*> wlldisppropcolmgr_( nullptr );
HiddenParam<uiWellDispPropDlg,Notifier<uiWellDispPropDlg>*>
		    wlldispsavereqmgr_( nullptr );
HiddenParam<uiWellDispPropDlg,Notifier<uiWellDispPropDlg>*>
		    wlldispapplytabreqmgr_( nullptr );
HiddenParam<uiWellDispPropDlg,Notifier<uiWellDispPropDlg>*>
		    wlldispresetallreqmgr_( nullptr );
HiddenParam<uiWellDispPropDlg,uiPushButton*> hp_applycurrenttoall_( nullptr );
HiddenParam<uiWellDispPropDlg,uiPushButton*> hp_resetall_( nullptr );
HiddenParam<uiWellDispPropDlg,uiButton*> hp_save_( nullptr );

#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* wd, bool is2d )
    : uiWellDispPropDlg(p,wd->multiID(),is2d,Color::NoColor())
{}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* wd,
				      bool is2d, Color bkCol )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(wd->name()),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(false)
			    .applytext(uiStrings::sReset()))
    , wd_(wd)
    , savedefault_(false)
    , is2ddisplay_(is2d)
    , applyAllReq(this)
{
    initDlg( bkCol );
}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const MultiID& wid,
				      bool is2d, Color bkCol )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(IOM().nameOf(wid)),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(false)
			    .applytext(uiStrings::sReset()))
    , savedefault_(false)
    , is2ddisplay_(is2d)
    , applyAllReq(this)
{
     wd_ = Well::MGR().get( wid, Well::LoadReqs( Well::LogInfos,
						 Well::Mrkrs,
						 is2d ? Well::DispProps2D :
							Well::DispProps3D ) );
     initDlg( bkCol );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
    wlldisppropcolmgr_.removeAndDeleteParam( this );
    wlldispsavereqmgr_.removeAndDeleteParam( this );
    wlldispapplytabreqmgr_.removeAndDeleteParam( this );
    wlldispresetallreqmgr_.removeAndDeleteParam( this );
    hp_applycurrenttoall_.removeParam( this );
    hp_resetall_.removeParam( this );
    hp_save_.removeParam( this );
    wd_->unRef();
}


void uiWellDispPropDlg::initDlg( Color bkCol )
{
    wlldisppropcolmgr_.setParam( this, new Color( bkCol ) );
    wlldispsavereqmgr_.setParam( this,
	new Notifier<uiWellDispPropDlg>( this ) );
    wlldispapplytabreqmgr_.setParam( this,
	new Notifier<uiWellDispPropDlg>( this ) );
    wlldispresetallreqmgr_.setParam( this,
	new Notifier<uiWellDispPropDlg>( this ) );

    wd_->ref();

    setCtrlStyle( CloseOnly );

    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );

    ts_ = new uiTabStack( this, "Well display properties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(),"Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Center log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Right log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );
    if ( !is2ddisplay_ )
	tgs += new uiGroup( ts_->tabGroup(), "Track properties" );

    Well::DisplayProperties::LogCouple& lc = *props.logs_[0];
    auto* wlp1 = new uiWellLogDispProperties( tgs[LeftLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), lc.left_, wd_ );
    auto* wlp2 = new uiWellLogDispProperties( tgs[CenterLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), lc.center(), wd_ );
    auto* wlp3 = new uiWellLogDispProperties( tgs[RightLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), lc.right_, wd_ );

    propflds_ += wlp1;
    propflds_ += wlp2;
    propflds_ += wlp3;

    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd_->markers().getColors( markercols );
    if ( !props.isValid() )
	props.setMarkersNms( markernms, false );

    uiWellDispProperties::Setup propsu =
	uiWellDispProperties::Setup(tr("Marker size"),tr("Marker color"))
	.onlyfor2ddisplay(is2ddisplay_);
    auto* wellprops = new uiWellMarkersDispProperties(
		tgs[Marker], propsu, props.markers_, markernms );
    wellprops->setAllMarkerNames( markernms, markercols );
    propflds_ += wellprops;

    if ( !is2ddisplay_ )
	propflds_ += new uiWellTrackDispProperties( tgs[Track],
			    uiWellDispProperties::Setup(), props.track_ );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropDlg::propChg );
	if ( sKey::Log() == propflds_[idx]->props().subjectName() )
	{
	    if ( idx==LeftLog )
		ts_->addTab( tgs[idx], is2ddisplay_ ? tr("Log 1")
						    : tr("Left Log") );
	    else if ( idx==CenterLog )
		ts_->addTab( tgs[idx], tr("Center Log") );
	    else if ( idx==RightLog )
		ts_->addTab( tgs[idx], is2ddisplay_ ? tr("Log 2")
						    : tr("Right Log") );
	}
	else
	    ts_->addTab( tgs[idx], toUiString(
				      propflds_[idx]->props().subjectName()) );
    }

    auto* bgrp = new uiButtonGroup( this, "", OD::Horizontal );
    hp_applycurrenttoall_.setParam( this, new uiPushButton( bgrp,
				    tr("Apply Current to all wells"), true ) );
    mAttachCB( applyTabButton()->activated, uiWellDispPropDlg::applyTabPush );
    hp_resetall_.setParam( this, new uiPushButton( bgrp,
						   tr("Reset all"), true ) );
    mAttachCB( resetAllButton()->activated, uiWellDispPropDlg::resetAllPush );
    bgrp->attach( centeredBelow, ts_ );

    TabType curtab = Track;
    if ( !lc.left_.name_.isEmpty() && lc.left_.name_ != sKey::None() )
	curtab = LeftLog;
    else if (!lc.center().name_.isEmpty() && lc.center().name_ != sKey::None())
	curtab = CenterLog;
    else if ( !lc.right_.name_.isEmpty() && lc.right_.name_ != sKey::None() )
	curtab = RightLog;

    ts_->setCurrentPage( curtab );
    mAttachCB( ts_->selChange(), uiWellDispPropDlg::tabSel );

    setWDNotifiers( true );
    mAttachCB( applyPushed, uiWellDispPropDlg::resetCB );
    mAttachCB( windowClosed, uiWellDispPropDlg::onClose );
    mAttachCB( postFinalise(), uiWellDispPropDlg::postFinaliseCB );
}


Color& uiWellDispPropDlg::backGroundColor()
{
    return *wlldisppropcolmgr_.getParam( this );
}


const Color& uiWellDispPropDlg::backGroundColor() const
{
    return *wlldisppropcolmgr_.getParam( this );
}


Notifier<uiWellDispPropDlg>& uiWellDispPropDlg::saveReq()
{
    return *wlldispsavereqmgr_.getParam( this );
}


Notifier<uiWellDispPropDlg>& uiWellDispPropDlg::applyTabReq()
{
    return *wlldispapplytabreqmgr_.getParam( this );
}


Notifier<uiWellDispPropDlg>& uiWellDispPropDlg::resetAllReq()
{
    return *wlldispresetallreqmgr_.getParam( this );
}


uiPushButton* uiWellDispPropDlg::applyTabButton() const
{
    return hp_applycurrenttoall_.getParam( this );
}


uiPushButton* uiWellDispPropDlg::resetAllButton() const
{
    return hp_resetall_.getParam( this );
}


uiButton* uiWellDispPropDlg::saveButton() const
{
    return hp_save_.getParam( this );
}


void uiWellDispPropDlg::postFinaliseCB( CallBacker* )
{
    mAttachCB( button(SAVE)->activated, uiWellDispPropDlg::saveAsDefaultCB );

    uiButton* closebut = button( CANCEL );
    hp_save_.setParam( this, uiButton::getStd( closebut->parent(),
		OD::Save, mCB(this,uiWellDispPropDlg,acceptOKCB), true ) );
    saveButton()->attach( leftOf, closebut );
    saveButton()->finalise();

    tabSel( nullptr );
    wdChg( nullptr );
    setNeedsSave( wellData()->displayProperties(is2D()).isModified() );
}


uiWellDispPropDlg::TabType uiWellDispPropDlg::currentTab() const
{
    return uiWellDispPropDlg::TabType( ts_->currentPageId() );
}


void uiWellDispPropDlg::tabSel( CallBacker* )
{
    const int curpageid = ts_->currentPageId();
    mDynamicCastGet(
	uiWellLogDispProperties*, curwelllogproperty, propflds_[curpageid] );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	if ( curwelllogproperty )
	   propflds_[idx]->curwelllogproperty_ = curwelllogproperty;
	else
	   propflds_[idx]->curwelllogproperty_ = nullptr;
    }
    const Well::Data* wd = wellData();
    if ( curwelllogproperty && wd )
	curwelllogproperty->setLogSet( &wd->logs() );
}


void uiWellDispPropDlg::updateLogs()
{
    const Well::Data* wd = wellData();
    if ( !wd )
	return;

    const Well::LogSet& wls = wd->logs();
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[idx]);
	if ( curwelllogproperty )
	    curwelllogproperty->setLogSet( &wls );
    }
}


bool uiWellDispPropDlg::needsSave() const
{
    uiButton* savebut = saveButton();
    if ( !savebut )
	return false;

    return savebut->isSensitive();
}


void uiWellDispPropDlg::setNeedsSave( bool yn )
{
    Well::Data* wd = wellData();
    if ( wd )
	wd->displayProperties( is2D() ).setModified( yn );

    saveButton()->setSensitive( yn );
    setButtonSensitive( APPLY, yn );
}


void uiWellDispPropDlg::setWDNotifiers( bool yn )
{
    if ( !wd_ ) return;

    if ( yn )
    {
	mAttachCB( mDispNot, uiWellDispPropDlg::wdChg );
	mAttachCB( wd_->logschanged, uiWellDispPropDlg::logsChgd );
    }
    else
    {
	mDetachCB( mDispNot, uiWellDispPropDlg::wdChg );
	mDetachCB( wd_->logschanged, uiWellDispPropDlg::logsChgd );
    }

    mAttachCB( wd_->markerschanged, uiWellDispPropDlg::markersChgd );
}


void uiWellDispPropDlg::getFromScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();
}


void uiWellDispPropDlg::putToScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


void uiWellDispPropDlg::markersChgd( CallBacker* )
{
    const Well::Data* wd = wellData();
    if ( !wd )
	return;

    BufferStringSet markernms;
    wd->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd->markers().getColors( markercols );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(uiWellMarkersDispProperties*,mrkrfld,propflds_[idx])
	if ( !mrkrfld )
	    continue;

	mrkrfld->setAllMarkerNames( markernms, markercols );
	return;
    }
}


void uiWellDispPropDlg::logsChgd( CallBacker* )
{
    const Well::Data* wd = wellData();
    if ( !wd )
	return;

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(uiWellLogDispProperties*,logsfld,propflds_[idx])
	if ( !logsfld )
	    continue;

	logsfld->setLogSet( &wd->logs() );
    }
}


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( mDispNot );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    const CallBack cb = mCB(this,uiWellDispPropDlg,wdChg);
    NotifyStopper ns( mDispNot, cb.cbObj() ); //Avoid circular cb
    getFromScreen();
    setNeedsSave( true );
    mDispNot.trigger();
}


void uiWellDispPropDlg::applyTabPush( CallBacker* cb )
{
    mDynamicCastGet( uiMultiWellDispPropDlg*, uimwdlg, this );
    if ( uimwdlg )
    {
	uimwdlg->applyMWTabPush( cb );
	return;
    }

    getFromScreen();
    applyTabReq().trigger();
}


void uiWellDispPropDlg::resetAllPush( CallBacker* cb )
{
    mDynamicCastGet( uiMultiWellDispPropDlg*, uimwdlg, this );
    if ( uimwdlg )
    {
	uimwdlg->resetMWAllPush( cb );
	return;
    }

    resetAllReq().trigger();
}


void uiWellDispPropDlg::saveAsDefaultCB( CallBacker* )
{
    if ( saveButtonChecked() )
	saveButton()->setSensitive( true );
    else
	saveButton()->setSensitive( button(APPLY)->isSensitive() );
}


void uiWellDispPropDlg::acceptOKCB( CallBacker* )
{
    saveReq().trigger();
    setSaveButtonChecked( false );
}


bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    if ( needsSave() )
    {
	const int res = uiMSG().askSave(
		tr("Display properties have changed"));
	if ( res == 1 )
	    saveReq().trigger();
	else if ( res == -1 )
	    return false;
	setNeedsSave( false );
    }

    return true;
}


void uiWellDispPropDlg::resetCB( CallBacker* )
{
    const Well::Data* wd = wellData();
    if ( !wd )
	return;

    Well::MGR().reloadDispPars( wd->multiID(), is2ddisplay_ );
    setNeedsSave( false );
}


void uiWellDispPropDlg::welldataDelNotify( CallBacker* )
{
    detachAllNotifiers();
    windowClosed.trigger();
    wd_ = nullptr;
    OBJDISP()->go( this );
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{ /* Will be removed */ }
void uiWellDispPropDlg::onClose( CallBacker* )
{ /* Will be removed */ }


//uiMultiWellDispPropDlg
uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
						ObjectSet<Well::Data>& wds,
						bool is2ddisplay )
    : uiMultiWellDispPropDlg(p,wds,is2ddisplay,Color::NoColor())
{}


uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
					const ObjectSet<Well::Data>& wds,
					bool is2ddisplay, Color bkcol )
    : uiWellDispPropDlg(p,const_cast<Well::Data*>(wds[0]),is2ddisplay,bkcol)
    , wds_(wds)
    , wellselfld_(nullptr)
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx<wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );

	wellselfld_ = new uiLabeledComboBox( this, tr("Select Well") );
	wellselfld_->box()->addItems( wellnames );
	mAttachCB( wellselfld_->box()->selectionChanged,
		   uiMultiWellDispPropDlg::wellSelChg );
	wellselfld_->attach( hCentered );
	ts_->attach( ensureBelow, wellselfld_ );
    }
    deepRef( wds_ );
}


uiMultiWellDispPropDlg::~uiMultiWellDispPropDlg()
{
    deepUnRef( wds_ );
}

void uiMultiWellDispPropDlg::resetProps( int logidx )
{
    resetProps( -1, logidx );
}


void uiMultiWellDispPropDlg::resetProps( int wellidx, int logidx )
{
    if ( !wds_.validIdx( wellidx ) ) return;
    RefMan<Well::Data> wd = wds_[wellidx];
    Well::DisplayProperties& prop = wd->displayProperties( is2ddisplay_ );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet( uiWellTrackDispProperties*,trckfld,propflds_[idx] );
	mDynamicCastGet( uiWellMarkersDispProperties*,mrkfld,propflds_[idx] );
	mDynamicCastGet( uiWellLogDispProperties*,logfld,propflds_[idx] );
	if ( logfld )
	{
	    if ( !prop.logs_.isEmpty() )
	    {
		if ( idx==LeftLog )
		    logfld->resetProps( prop.logs_[logidx]->left_ );
		else if ( idx==CenterLog )
		    logfld->resetProps( prop.logs_[logidx]->center() );
		else if ( idx==RightLog )
		    logfld->resetProps( prop.logs_[logidx]->right_ );
	    }
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track_ );
	else if ( mrkfld )
	{
	    BufferStringSet markernms;
	    TypeSet<Color> markercols;
	    wd->markers().getNames( markernms );
	    wd->markers().getColors( markercols );

	    mrkfld->setAllMarkerNames( markernms, markercols );
	    mrkfld->resetProps( prop.markers_ );
	}
    }
    putToScreen();
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    if ( wd_ )
	wd_->unRef();

    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    uiWellDispPropDlg::setWDNotifiers( false );
    wd_ = wds_[selidx];
    if ( wd_ )
	wd_->ref();

    setCaption( tr("Display properties of: %1").arg(wd_->name()) );
    uiWellDispPropDlg::setWDNotifiers( true );
    resetProps( selidx, 0 );
}


void uiMultiWellDispPropDlg::setWDNotifiers( bool yn )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( yn )
	    mAttachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
	else
	    mDetachCB( mDispNot, uiMultiWellDispPropDlg::wdChg );
    }

    wd_ = curwd;
}


void uiMultiWellDispPropDlg::onClose( CallBacker* )
{
}


void uiMultiWellDispPropDlg::applyMWTabPush( CallBacker* )
{
    getFromScreen();
    const bool is2d = is2D();
    const TabType pageid = currentTab();
    Well::Data* curwd = wd_;
    const Well::DisplayProperties& edprops = curwd->displayProperties();
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( wd_ && wd_!=curwd )
	{
	    const MultiID wllkey = wd_->multiID();
	    Well::DisplayProperties& wdprops = wd_->displayProperties(is2d);
	    switch ( pageid )
	    {
		case uiWellDispPropDlg::Track:
		    wdprops.setTrack( edprops.track_ );
		    break;
		case uiWellDispPropDlg::Marker:
		    wdprops.setMarkers( wd_, edprops.markers_ );
		    break;
		case uiWellDispPropDlg::LeftLog:
		    wdprops.setLeftLog( wd_, edprops.logs_[0]->left_ );
		    break;
		case uiWellDispPropDlg::CenterLog:
		    wdprops.setCenterLog( wd_, edprops.logs_[0]->center() );
		    break;
		case uiWellDispPropDlg::RightLog:
		    wdprops.setRightLog( wd_, edprops.logs_[0]->right_ );
	    }
	    mDispNot.trigger();
	}
    }
    wd_ = curwd;
}


void uiMultiWellDispPropDlg::resetMWAllPush( CallBacker* )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	resetCB( nullptr );
    }
    wd_ = curwd;
}
