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
#include "uicombobox.h"
#include "uitabbar.h"
#include "uitabstack.h"
#include "uiseparator.h"
#include "uistrings.h"

#include "keystrs.h"
#include "objdisposer.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellmarker.h"
#include "od_helpids.h"

#define mDispNotif wd_->displayProperties(is2ddisplay_).objectChanged()


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* wd,
				      bool is2ddisplay, bool multipanel )
	: uiDialog(p,uiDialog::Setup(tr("Display properties of: %1")
				     .arg(wd ? toUiString(wd->name())
					     : uiString::empty()),
				     mNoDlgTitle,
				     mODHelpKey(mWellDispPropDlgHelpID) )
		     .savebutton(true)
		     .savechecked(false)
		     .modal(false))
	, wd_(wd)
	, applyAllReq(this)
	, savedefault_(false)
	, is2ddisplay_(is2ddisplay)
	, multipanel_(multipanel)
{
    setCtrlStyle( CloseOnly );

    ts_ = new uiTabStack( this, "Well display properties tab stack" );

    if ( multipanel_ )
    {
	ts_->setTabsClosable( true );
	mAttachCB( ts_->tabToBeClosed, uiWellDispPropDlg::tabRemovedCB );
	createMultiPanelUI();
    }
    else
	createSinglePanelUI();

    ts_->selChange().notify( mCB(this,uiWellDispPropDlg,tabSel) );
}


void uiWellDispPropDlg::createSinglePanelUI()
{
    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(),"Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Right log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );
    if ( !is2ddisplay_ )
	tgs += new uiGroup( ts_->tabGroup(), "Track properties" );

    uiWellLogDispProperties* wlp1 = new uiWellLogDispProperties( tgs[0],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), props.log(true), &(wd_->logs()) );
    uiWellLogDispProperties* wlp2 = new uiWellLogDispProperties( tgs[1],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2ddisplay_), props.log(false), &(wd_->logs()) );

    propflds_ += wlp1;
    propflds_ += wlp2;

    BufferStringSet allmarkernms;
    wd_->markers().getNames( allmarkernms );

    propflds_ += new uiWellMarkersDispProperties( tgs[2],
	uiWellDispProperties::Setup( tr("Marker size"), tr("Marker color") )
	.onlyfor2ddisplay(is2ddisplay_), props.markers(), allmarkernms );

    if ( !is2ddisplay_ )
	propflds_ += new uiWellTrackDispProperties( tgs[3],
			    uiWellDispProperties::Setup(), props.track() );

    bool foundlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropDlg::propChg );
	if ( sKey::Log() == propflds_[idx]->props().subjectName() )
	{
	    ts_->addTab( tgs[idx], foundlog ? is2ddisplay_ ? tr("Log 2")
						   : tr("Right Log")
					    : is2ddisplay_ ? tr("Log 1")
						   : tr("Left Log") );
	    foundlog = true;
	}
	else
	    ts_->addTab( tgs[idx], toUiString(
				      propflds_[idx]->props().subjectName()) );
    }

    uiPushButton* applbut = new uiPushButton( this, tr("Apply to all wells"),
			mCB(this,uiWellDispPropDlg,applyAllPush), true );
    applbut->attach( centeredBelow, ts_ );

    ts_->selChange().notify( mCB(this,uiWellDispPropDlg,tabSel) );

    setWDNotifiers( true );
    mAttachCB( windowClosed, uiWellDispPropDlg::onClose );

    tabSel( 0 );
}


void uiWellDispPropDlg::createMultiPanelUI()
{
    addPanel();
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
}


//uiPanelTab
uiPanelTab::uiPanelTab( uiParent* grp, Well::Data& welldata,
			const char* panelnm, const bool is2ddisplay)
    : uiGroup(grp,panelnm)
    , welldata_(welldata)
    , is2ddisp_(is2ddisplay)
{
    init( this );
}


void uiPanelTab::init( uiGroup* grp )
{
    logts_ = new uiTabStack( grp, "Log properties" );
    logts_->setTabsClosable( true );
    addLogPanel();
    lognmChg( 0 );
    mAttachCB( logts_->selChange(), uiPanelTab::logTabSelChgngeCB );
}


uiGroup* uiPanelTab::createLogPropertiesGrp()
{
    Well::DisplayProperties& props =
				welldata_.displayProperties( is2ddisp_ );
    uiGroup* logtabgrp = logts_->tabGroup();
    uiWellLogDispProperties* wlp = new uiWellLogDispProperties( logtabgrp,
    uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color") )
	.onlyfor2ddisplay(is2ddisp_), props.log(true), &(welldata_.logs()) );
    logpropflds_ += wlp;
    wlp->setName( "Log properties" );
    mAttachCB( wlp->propChanged, uiPanelTab::lognmChg );
    return wlp;
}


void uiPanelTab::addLogPanel()
{
    uiGroup* logtabgrp = createLogPropertiesGrp();
    if ( !logts_->size() )
    {
	logts_->addTab( logtabgrp );

	uiGroup* addlogtabgrp = new uiGroup( logts_->tabGroup(),
					     OD::String::empty() );
	logts_->addTab( addlogtabgrp );
	logts_->setTabIcon( addlogtabgrp, "plus" );
	logts_->showCloseButton( addlogtabgrp, false, true );
	return;
    }

    const int curtabid = logts_->currentPageId();
    const int tabid = logts_->insertTab( logtabgrp, curtabid,
					 tr("Log properties") );
    logts_->setCurrentPage( tabid );
}


void uiPanelTab::logTabSelChgngeCB( CallBacker* cb )
{
    const int logtabsz = logts_->size();
    const int curtabid = logts_->currentPageId();
    if ( curtabid == logtabsz-1 )
	addLogPanel();

    lognmChg( 0 );
}


void uiPanelTab::lognmChg( CallBacker* )
{
    uiGroup* curloggrp = logts_->currentPage();
    const int curlogtab = logts_->indexOf( curloggrp );
    uiWellLogDispProperties* curlogdisp = logpropflds_.get( curlogtab );
    if ( !curlogdisp )
	return;

    logts_->setTabText( curlogtab, curlogdisp->logName() );
}
//uiPanelTab


void uiWellDispPropDlg::addPanel()
{
    BufferString paneltxt( "Panel", ts_->size() ? ts_->size()-1 : 1 );
    uiPanelTab* paneltabgrp = new uiPanelTab( ts_->tabGroup(), *wd_,
					      paneltxt, is2ddisplay_);
    if ( !ts_->size() )
    {
	ts_->addTab( paneltabgrp );

	addMarkersPanel();

	uiGroup* addpaneltabgrp = new uiGroup( ts_->tabGroup(),
					       OD::String::empty() );
	ts_->addTab( addpaneltabgrp );
	ts_->setTabIcon( ts_->size()-1, "plus" );
	ts_->showCloseButton( addpaneltabgrp, false, true );
	return;
    }

    const int curtabid = ts_->currentPageId();
    const int tabid = ts_->insertTab( paneltabgrp, curtabid-1,
				      tr(paneltxt.buf()) );
    ts_->setCurrentPage( tabid );
}


void uiWellDispPropDlg::tabRemovedCB( CallBacker* cb )
{
    updatePanelNames();
}


void uiWellDispPropDlg::showTabCloseButtons()
{
    return;
    //TODO show close tab button if ts_ size is more than 3
/*
    const bool showclosebut = ts_->size() > 3;
    for ( int pidx=0; pidx<ts_->size(); pidx++ )
    {
	uiGroup* panelgrp = ts_->page( pidx );
	ts_->showCloseButton( panelgrp, showclosebut, false );
    }
*/
}


void uiWellDispPropDlg::updatePanelNames()
{
    int paneltabsz = ts_->size();
    for ( int pidx=0; pidx<paneltabsz; pidx++ )
    {
	BufferString panelstr( "Panel" );
	if ( pidx == paneltabsz-1 || pidx == paneltabsz-2 )
	    continue;

	uiGroup* panelgrp = ts_->page( pidx );
	panelstr.add( pidx+1 );
	panelgrp->setName( panelstr );
	ts_->setTabText( pidx, panelstr );
    }
}


void uiWellDispPropDlg::addMarkersPanel()
{
    BufferStringSet allmarkernms;
    wd_->markers().getNames( allmarkernms );
    uiGroup* mrkrspanelgrp = new uiGroup( ts_->tabGroup(),
					  "Marker properties" );
    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );
    uiWellDispProperties::Setup mrkrsetup =
	uiWellDispProperties::Setup( tr("Marker size"), tr("Marker color") )
			      .onlyfor2ddisplay(true);
    uiWellMarkersDispProperties* mrkrs =
	new uiWellMarkersDispProperties( mrkrspanelgrp, mrkrsetup,
					    props.markers(), allmarkernms );
    ts_->addTab( mrkrspanelgrp,
		 toUiString( mrkrs->mrkprops().subjectName()) );
    ts_->showCloseButton( mrkrspanelgrp, false, true );
}


void uiWellDispPropDlg::tabSel(CallBacker*)
{
    if ( multipanel_ )
    {
	const int paneltabsz = ts_->size();
	const int curtabid = ts_->currentPageId();
	if ( curtabid == paneltabsz-1 )
	    addPanel();

	return;
    }

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	int curpageid = ts_->currentPageId();
	if ( curpageid > 1 )
	    return;

	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[curpageid]);
	if ( curwelllogproperty )
	   propflds_[idx]->curwelllogproperty_ =  curwelllogproperty;
	else
	   propflds_[idx]->curwelllogproperty_ = 0;
    }
}


void uiWellDispPropDlg::updateLogs()
{
    const Well::LogSet& wls = wd_->logs();
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[idx]);
	if ( curwelllogproperty )
	    curwelllogproperty->setLogSet( &wls );
    }
}


void uiWellDispPropDlg::setWDNotifiers( bool yn )
{
    if ( !wd_ ) return;

    if ( yn )
    {
	mAttachCB( mDispNotif, uiWellDispPropDlg::wdChg );
    }
    else
    {
	mDetachCB( mDispNotif, uiWellDispPropDlg::wdChg );
    }
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


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( mDispNotif );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    if ( multipanel_ )
	return; //TODO

    getFromScreen();
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{
    getFromScreen();
    applyAllReq.trigger();
}


void uiWellDispPropDlg::welldataDelNotify( CallBacker* )
{
    windowClosed.trigger();
    wd_ = 0;
    OBJDISP()->go( this );
}


void uiWellDispPropDlg::onClose( CallBacker* )
{
}


bool uiWellDispPropDlg::rejectOK()
{
    savedefault_ = saveButtonChecked();
    return true;
}


//uiMultiWellDispPropDlg
uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
					       const ObjectSet<Well::Data>& wds,
					       bool is2ddisplay )
	: uiWellDispPropDlg(p,const_cast<Well::Data*>(wds[0]),is2ddisplay,true)
	, wds_(wds)
	, wellselfld_(0)
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx< wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );

	wellselfld_ = new uiLabeledComboBox( this, uiStrings::sWell() );
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
    bool first = true;
    Well::DisplayProperties& prop = wd_->displayProperties( is2ddisplay_ );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet( uiWellTrackDispProperties*,trckfld,propflds_[idx] );
	mDynamicCastGet( uiWellMarkersDispProperties*,mrkfld,propflds_[idx] );
	mDynamicCastGet( uiWellLogDispProperties*,logfld,propflds_[idx] );
	if ( logfld )
	{
	    logfld->setLogSet( &wd_->logs() );
	    logfld->resetProps( prop.log(first,logidx) );
	    first = false;
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track() );
	else if ( mrkfld )
	{
	    BufferStringSet allmarkernms;
	    wd_->markers().getNames( allmarkernms );
	    mrkfld->setAllMarkerNames( allmarkernms );
	    mrkfld->resetProps( prop.markers() );
	}
    }
    putToScreen();
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    wd_ = wds_[selidx];
    resetProps( 0 );
}


void uiMultiWellDispPropDlg::setWDNotifiers( bool yn )
{
    Well::Data* curwd = wd_;
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	wd_ = wds_[idx];
	if ( yn )
	    mAttachCB( mDispNotif, uiMultiWellDispPropDlg::wdChg );
	else
	    mDetachCB( mDispNotif, uiMultiWellDispPropDlg::wdChg );
    }

    wd_ = curwd;
}


void uiMultiWellDispPropDlg::onClose( CallBacker* )
{
}
