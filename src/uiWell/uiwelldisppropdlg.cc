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
#include "uiseparator.h"
#include "uitabbar.h"
#include "uitabstack.h"

#include "objdisposer.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellmarker.h"


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
	, savedefault_(false)
{
    setCtrlStyle( CloseOnly );
    welldisppropgrp_ = new uiWellDispPropGrp( this, wd, is2ddisplay,
					      multipanel );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
}


bool uiWellDispPropDlg::rejectOK()
{
    savedefault_ = saveButtonChecked();
    return true;
}


//uiPanelTab
uiPanelTab::uiPanelTab( uiParent* grp, Well::Data& welldata,
			const char* panelnm, const bool is2ddisplay )
    : uiTabStack(grp,panelnm)
    , welldata_(welldata)
    , is2ddisp_(is2ddisplay)
{
    init();
}


uiPanelTab::~uiPanelTab()
{
    detachAllNotifiers();
}


void uiPanelTab::init()
{
    setTabsClosable( true );
    addLogPanel();
    mAttachCB( tabToBeClosed, uiPanelTab::logTabToBeClosedCB );
    mAttachCB( tabClosed, uiPanelTab::logTabClosedCB );
    mAttachCB( selChange(), uiPanelTab::logTabSelChgngeCB );

    mDynamicCastGet(uiWellLogDispProperties*, wlpgrp, currentPage())
    if ( !wlpgrp )
	return;

    wlpgrp->propChanged.trigger();
}


uiGroup* uiPanelTab::createLogPropertiesGrp()
{
    Well::DisplayProperties& props =
				welldata_.displayProperties( is2ddisp_ );
    uiGroup* logtabgrp = tabGroup();
    uiWellLogDispProperties* wlp = new uiWellLogDispProperties( logtabgrp,
    uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color") )
	.onlyfor2ddisplay(is2ddisp_), props.log(true), &(welldata_.logs()) );
    wlp->setName( "Log properties" );
    mAttachCB( wlp->propChanged, uiPanelTab::logpropChg );
    return wlp;
}


void uiPanelTab::logpropChg( CallBacker* )
{
    uiGroup* curloggrp = currentPage();
    mDynamicCastGet(uiWellLogDispProperties*, logdispprop, curloggrp)
    if ( !logdispprop )
	return;

    logdispprop->putToScreen();

    const int curlogtabid = indexOf( curloggrp );
    BufferString curtabnm( curloggrp->name() );
    if ( curtabnm.isEqual(logdispprop->logName()) )
	return;

    setTabText( curlogtabid, logdispprop->logName() );
    curloggrp->setName( logdispprop->logName() );
}


void uiPanelTab::logTabToBeClosedCB( CallBacker* cb )
{
    setCurrentPage( 0 );
}


void uiPanelTab::logTabClosedCB( CallBacker* cb )
{
    setCurrentPage( 0 );
    showLogTabCloseButton();
}


void uiPanelTab::showLogTabCloseButton()
{
    const bool showclosebut = size() > 2;
    uiGroup* panelgrp = page( 0 );
    showCloseButton( panelgrp, showclosebut, false );
}


void uiPanelTab::addLogPanel()
{
    uiGroup* logtabgrp = createLogPropertiesGrp();
    if ( !size() )
    {
	addTab( logtabgrp );

	uiGroup* addlogtabgrp = new uiGroup( tabGroup(), OD::String::empty() );
	addTab( addlogtabgrp );
	setTabIcon( addlogtabgrp, "plus" );
	showCloseButton( addlogtabgrp, false, true );
	showLogTabCloseButton();
	return;
    }

    const int curtabid = currentPageId();
    const int tabid = insertTab( logtabgrp, curtabid, tr("Log properties") );
    setCurrentPage( tabid );
    showLogTabCloseButton();
}


void uiPanelTab::logTabSelChgngeCB( CallBacker* cb )
{
    const int logtabsz = size();
    const int curtabid = currentPageId();
    if ( curtabid == logtabsz-1 )
	addLogPanel();

    uiGroup* grp = currentPage();
    mDynamicCastGet(uiWellLogDispProperties*, wlpgrp, grp)
    if ( !wlpgrp )
	return;
}


//uiWellDispPropGrp
uiWellDispPropGrp::uiWellDispPropGrp( uiParent* p, Well::Data* wd,
				      bool is2ddisplay, bool multipanel )
	: uiGroup(p,"Display properties")
	, wd_(wd)
	, applyAllReq(this)
	, is2ddisplay_(is2ddisplay)
	, multipanel_(multipanel)
{

    ts_ = new uiTabStack( this, "Well display properties tab" );

    if ( multipanel_ )
    {
	ts_->setTabsClosable( true );
	mAttachCB( ts_->tabClosed, uiWellDispPropGrp::tabRemovedCB );
	createMultiPanelUI();
    }
    else
	createSinglePanelUI();

    ts_->selChange().notify( mCB(this,uiWellDispPropGrp,tabSel) );
}


//TODO Needs to do changes in case of multipanel_=false;
void uiWellDispPropGrp::createSinglePanelUI()
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

    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd_->markers().getColors( markercols );

    uiWellDispProperties::Setup propsu =
	uiWellDispProperties::Setup(tr("Marker size"),tr("Marker color"))
	.onlyfor2ddisplay(is2ddisplay_);
    uiWellMarkersDispProperties* wellprops = new uiWellMarkersDispProperties(
	tgs[2], propsu, props.markers(), markernms );
    wellprops->setAllMarkerNames( markernms, markercols );
    propflds_ += wellprops;

    if ( !is2ddisplay_ )
	propflds_ += new uiWellTrackDispProperties( tgs[3],
			    uiWellDispProperties::Setup(), props.track() );

    bool foundlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropGrp::propChg );
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
			mCB(this,uiWellDispPropGrp,applyAllPush), true );
    applbut->attach( centeredBelow, ts_ );

    ts_->selChange().notify( mCB(this,uiWellDispPropGrp,tabSel) );
    tabSel( 0 );
}


void uiWellDispPropGrp::createMultiPanelUI()
{
    addPanel();
}


uiWellDispPropGrp::~uiWellDispPropGrp()
{
    detachAllNotifiers();
}

void uiWellDispPropGrp::addPanel()
{
    BufferString paneltxt( "Panel", ts_->size() ? ts_->size()-1 : 1 );
    uiPanelTab* paneltabgrp = new uiPanelTab( ts_->tabGroup(), *wd_,
					      paneltxt, is2ddisplay_ );
    if ( !ts_->size() )
    {
	ts_->addTab( paneltabgrp );

	addMarkersPanel();

	uiGroup* addpaneltabgrp = new uiGroup( ts_->tabGroup(),
					       OD::String::empty() );
	ts_->addTab( addpaneltabgrp );
	ts_->setTabIcon( ts_->size()-1, "plus" );
	ts_->showCloseButton( addpaneltabgrp, false, true );
	showPanelTabCloseButton();
	return;
    }

    const int curtabid = ts_->currentPageId();
    const int tabid = ts_->insertTab( paneltabgrp, curtabid-1,
				      tr(paneltxt.buf()) );
    ts_->setCurrentPage( tabid );
    showPanelTabCloseButton();
}


void uiWellDispPropGrp::tabRemovedCB( CallBacker* cb )
{
    updatePanelNames();
    showPanelTabCloseButton();
}


void uiWellDispPropGrp::showPanelTabCloseButton()
{
    const bool showclosebut = ts_->size() > 3;
    uiGroup* panelgrp = ts_->page( 0 );
    ts_->showCloseButton( panelgrp, showclosebut, false );
}


void uiWellDispPropGrp::updatePanelNames()
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


void uiWellDispPropGrp::addMarkersPanel()
{
    BufferStringSet allmarkernms;
    wd_->markers().getNames( allmarkernms );
    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );
    uiGroup* mrkrspanelgrp = ts_->tabGroup();
    uiWellDispProperties::Setup mrkrsetup =
	uiWellDispProperties::Setup( tr("Marker size"), tr("Marker color") )
			      .onlyfor2ddisplay(true);
    uiWellMarkersDispProperties* mrkrs =
	new uiWellMarkersDispProperties( mrkrspanelgrp, mrkrsetup,
					 props.markers(), allmarkernms );
    ts_->addTab( mrkrs, toUiString( mrkrs->mrkprops().subjectName()) );
    ts_->showCloseButton( mrkrspanelgrp, false, true );
    mAttachCB( mrkrs->propChanged, uiWellDispPropGrp::markerpropChg );
}


void uiWellDispPropGrp::markerpropChg( CallBacker* )
{
    //TODO
    uiGroup* curloggrp = ts_->currentPage();
    mDynamicCastGet(uiWellMarkersDispProperties*, mrkrdispprop, curloggrp)
    if ( !mrkrdispprop )
	return;

    Well::DisplayProperties& prop = wd_->displayProperties( is2ddisplay_ );
    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd_->markers().getColors( markercols );
    mrkrdispprop->setAllMarkerNames( markernms, markercols );
    mrkrdispprop->resetProps( prop.markers() );
    mrkrdispprop->putToScreen();
}


void uiWellDispPropGrp::tabSel(CallBacker*)
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
	    uiWellLogDispProperties*, curwelllogproperty, propflds_[curpageid]);
	if ( curwelllogproperty )
	   propflds_[idx]->curwelllogproperty_ =  curwelllogproperty;
	else
	   propflds_[idx]->curwelllogproperty_ = 0;
    }
}


void uiWellDispPropGrp::updateLogs()
{
    if ( multipanel_ )
	return;

    const Well::LogSet& wls = wd_->logs();
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(
	    uiWellLogDispProperties*, curwelllogproperty,propflds_[idx]);
	if ( curwelllogproperty )
	    curwelllogproperty->setLogSet( &wls );
    }
}


void uiWellDispPropGrp::getFromScreen()
{
    if ( multipanel_ )
	return;

    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();
}


void uiWellDispPropGrp::propChg( CallBacker* )
{
    if ( multipanel_ )
	return;

    getFromScreen();
}


void uiWellDispPropGrp::applyAllPush( CallBacker* )
{
    if ( multipanel_ )
	return; //TODO

    getFromScreen();
    applyAllReq.trigger();
}


void uiWellDispPropGrp::welldataDelNotify( CallBacker* )
{
    wd_ = 0;
    OBJDISP()->go( this );
}


//uiMultiWellDispPropGrp
uiMultiWellDispPropGrp::uiMultiWellDispPropGrp( uiParent* p,
					       const ObjectSet<Well::Data>& wds,
					       bool is2ddisplay )
	: uiGroup(p,"Multi Well Group")
	, wds_(wds)
	, wellselfld_(0)
{
    BufferStringSet wellnames;
    for ( auto wd : wds_ )
	wellnames.addIfNew( wd->name() );

    wellselfld_ = new uiLabeledComboBox( this, uiStrings::sWell() );
    wellselfld_->box()->addItems( wellnames );
    mAttachCB( wellselfld_->box()->selectionChanged,
	       uiMultiWellDispPropGrp::wellSelChg );
    wellselfld_->attach( hCentered );
    uiSeparator* sp = new uiSeparator( this, "" );
    sp->attach( stretchedBelow, wellselfld_ );

    for ( auto wd : wds )
    {
	uiWellDispPropGrp* wdpg =
	    new uiWellDispPropGrp( this, const_cast<Well::Data*>(wd),
				   is2ddisplay, true );
	welldisppropgrps_ += wdpg;
	wdpg->attach( ensureBelow, sp );
	wdpg->display( false );
    }

    welldisppropgrps_.get(0)->display( true );

    deepRef( wds_ );
}


uiMultiWellDispPropGrp::~uiMultiWellDispPropGrp()
{
    deepUnRef( wds_ );
}


int uiMultiWellDispPropGrp::curWellID()
{
    return wellselfld_->box()->currentItem();
}


uiWellDispPropGrp* uiMultiWellDispPropGrp::curWellDispPropGrp()
{
    const auto selidx = curWellID();
    return welldisppropgrps_.get( selidx );
}


void uiMultiWellDispPropGrp::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_->box()->currentItem();
    for ( int widx=0; widx<wds_.size(); widx++ )
    {
	uiWellDispPropGrp* wdpg = welldisppropgrps_.get( widx );
	wdpg->display( selidx==widx );
    }
}
