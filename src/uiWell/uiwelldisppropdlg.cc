/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Nov 2008
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
#include "wellwriter.h"


#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)

uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const MultiID& wid,
				      bool is2d, OD::Color bkCol )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(IOM().nameOf(wid)),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(false)
			    .applytext(uiStrings::sReset()))
    , is2ddisplay_(is2d)
    , bkcol_(bkCol)
    , saveReq(this)
    , applyTabReq(this)
    , resetAllReq(this)
{
    wd_ = Well::MGR().get( wid, Well::LoadReqs( Well::LogInfos,
						Well::Mrkrs,
						is2d ? Well::DispProps2D :
						       Well::DispProps3D ) );
    initDlg( bkCol );
}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* wd,
				      bool is2d, OD::Color bkCol )
    : uiDialog(p,uiDialog::Setup(
			tr("Display properties of: %1").arg(wd->name()),
			mNoDlgTitle, mODHelpKey(mWellDispPropDlgHelpID) )
			    .savebutton(true).savechecked(false)
			    .applybutton(true).modal(false)
			    .applytext(uiStrings::sReset()))
    , wd_(wd)
    , is2ddisplay_(is2d)
    , saveReq(this)
    , applyTabReq(this)
    , resetAllReq(this)
{
    initDlg( bkCol );
}


void uiWellDispPropDlg::initDlg( OD::Color bkCol )
{
    wd_->ref();
    setCtrlStyle( RunAndClose );
    setOkText( uiStrings::sSave() );

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
	.onlyfor2ddisplay(is2ddisplay_), lc.center_, wd_ );
    auto* wlp3 = new uiWellLogDispProperties( tgs[RightLog],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), lc.right_, wd_ );

    propflds_ += wlp1;
    propflds_ += wlp2;
    propflds_ += wlp3;

    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<OD::Color> markercols;
    wd_->markers().getColors( markercols );
    if ( !props.isValid() )
	props.setMarkerNames( markernms, false );

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
    new uiPushButton( bgrp, tr("Apply Current to all wells"),
			mCB(this,uiWellDispPropDlg,applyTabPush), true );
    new uiPushButton( bgrp, tr("Reset all"),
			mCB(this,uiWellDispPropDlg,resetAllPush), true );
    bgrp->attach( centeredBelow, ts_ );

    TabType curtab = Track;
    if ( !lc.left_.name_.isEmpty() && lc.left_.name_ != sKey::None() )
	curtab = LeftLog;
    else if ( !lc.center_.name_.isEmpty() && lc.center_.name_ != sKey::None() )
	curtab = CenterLog;
    else if ( !lc.right_.name_.isEmpty() && lc.right_.name_ != sKey::None() )
	curtab = RightLog;

    ts_->setCurrentPage( curtab );
    mAttachCB( ts_->selChange(), uiWellDispPropDlg::tabSel );

    setWDNotifiers( true );
    mAttachCB( applyPushed, uiWellDispPropDlg::resetCB );
    mAttachCB( postFinalize(), uiWellDispPropDlg::postFinalizeCB );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
    wd_->unRef();
}


void uiWellDispPropDlg::postFinalizeCB( CallBacker* )
{
    mAttachCB( button(SAVE)->activated, uiWellDispPropDlg::saveAsDefaultCB );

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


bool uiWellDispPropDlg::needsSave() const
{
    return isButtonSensitive( OK );
}


void uiWellDispPropDlg::setNeedsSave( bool yn )
{
    Well::Data* wd = wellData();
    if ( wd )
	wd->displayProperties( is2D() ).setModified( yn );

    setButtonSensitive( OK, yn );
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
    TypeSet<OD::Color> markercols;
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


void uiWellDispPropDlg::applyTabPush( CallBacker* )
{
    getFromScreen();
    applyTabReq.trigger();
}


void uiWellDispPropDlg::resetAllPush( CallBacker* )
{
    resetAllReq.trigger();
}


void uiWellDispPropDlg::saveAsDefaultCB( CallBacker* )
{
    if ( saveButtonChecked() )
	setButtonSensitive( OK, true );
    else
	setButtonSensitive( OK, isButtonSensitive(APPLY) );
}


bool uiWellDispPropDlg::acceptOK( CallBacker* )
{
    saveReq.trigger();
    setSaveButtonChecked( false );
    return false;
}


bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    if ( needsSave() )
    {
	const int res = uiMSG().askSave(
		tr("Display properties have changed"));
	if ( res == 1 )
	    saveReq.trigger();
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



//uiMultiWellDispPropDlg
uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
					const ObjectSet<Well::Data>& wds,
					bool is2ddisplay, OD::Color bkcol )
    : uiWellDispPropDlg(p,const_cast<Well::Data*>(wds[0]),is2ddisplay,bkcol)
    , wds_(wds)
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
	if ( trckfld )
	    trckfld->resetProps(
		 const_cast<Well::DisplayProperties::Track&>(prop.getTrack()) );
	else if ( mrkfld )
	{
	    BufferStringSet markernms;
	    TypeSet<OD::Color> markercols;
	    wd->markers().getNames( markernms );
	    wd->markers().getColors( markercols );

	    mrkfld->setAllMarkerNames( markernms, markercols );
	    mrkfld->resetProps( const_cast<Well::DisplayProperties::Markers&>(
						prop.getMarkers()) );
	}
	else if ( logfld )
	{
	    if ( prop.isValidLogPanel(logidx) )
	    {
		Well::DisplayProperties::LogCouple& logs =
		    const_cast<Well::DisplayProperties::LogCouple&>(
						    prop.getLogs(logidx) );
		if ( idx==LeftLog )
		    logfld->resetProps( logs.left_ );
		else if ( idx==CenterLog )
		    logfld->resetProps( logs.center_ );
		else if ( idx==RightLog )
		    logfld->resetProps( logs.right_ );
	    }
	}
    }
    putToScreen();
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    uiWellDispPropDlg::setWDNotifiers( false );
    wd_ = wds_[selidx];

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


void uiMultiWellDispPropDlg::applyTabPush( CallBacker* )
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
		    wdprops.setTrack( edprops.getTrack() );
		    break;
		case uiWellDispPropDlg::Marker:
		    wdprops.setMarkers( wd_, edprops.getMarkers() );
		    break;
		case uiWellDispPropDlg::LeftLog:
		    wdprops.setLeftLog( wd_, edprops.getLogs().left_ );
		    break;
		case uiWellDispPropDlg::CenterLog:
		    wdprops.setCenterLog( wd_, edprops.getLogs().center_ );
		    break;
		case uiWellDispPropDlg::RightLog:
		    wdprops.setRightLog( wd_, edprops.getLogs().right_ );
	    }
	    mDispNot.trigger();
	}
    }
    wd_ = curwd;
    allapplied_ = true; //???
}


void uiMultiWellDispPropDlg::resetAllPush( CallBacker* )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	resetCB( nullptr );
    }
    wd_ = curwd;
    allapplied_ = false; //???
    setNeedsSave( false );
}


bool uiMultiWellDispPropDlg::acceptOK( CallBacker* )
{
    if ( saveButtonChecked() )
    {
	const Well::DisplayProperties& edprops = wd_->displayProperties(is2D());
	edprops.defaults() = edprops;
	edprops.commitDefaults();
    }

    if ( allapplied_ )
    {
	saveAllWellDispProps();
	allapplied_ = false;
    }
    else
    {
	saveWellDispProps( wd_ );
	setNeedsSave( dispPropsChanged() );
    }

    return false;
}


void uiMultiWellDispPropDlg::saveAllWellDispProps()
{
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	ConstRefMan<Well::Data> curwd( wds_[idwell] );
	if ( curwd )
	    saveWellDispProps( curwd.ptr() );
    }
    setNeedsSave( false );
}


void uiMultiWellDispPropDlg::saveWellDispProps( const Well::Data* wd )
{
    Well::Writer wr( wd->multiID(), *wd );
    if ( !wr.putDispProps() )
	uiMSG().error(tr("Could not write display properties for \n%1")
		    .arg(wd->name()));
    else
	Well::MGR().reloadDispPars( wd->multiID(), is2D() );
}




bool uiMultiWellDispPropDlg::dispPropsChanged( int wellidx ) const
{
    if ( !wds_.validIdx( wellidx ) ) return false;

    Well::LoadReqs lreqs( is2D() ? Well::DispProps2D : Well::DispProps3D );
    ConstRefMan<Well::Data> rwd = Well::MGR().get( wds_[wellidx]->multiID(),
						   lreqs );
    return rwd->displayProperties(is2D())!=
				    wds_[wellidx]->displayProperties(is2D());
}


bool uiMultiWellDispPropDlg::dispPropsChanged() const
{
    for ( int idx=0; idx<wds_.size(); idx++ )
	if ( dispPropsChanged(idx) )
	    return true;

    return false;
}
