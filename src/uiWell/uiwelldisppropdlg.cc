/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uitabstack.h"
#include "uiseparator.h"

#include "ioman.h"
#include "keystrs.h"
#include "objdisposer.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellman.h"
#include "wellmarker.h"
#include "od_helpids.h"

#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)

uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* wd, bool is2d )
	: uiDialog(p,uiDialog::Setup(tr("Display properties of: %1")
				     .arg(wd ? toUiString(wd->name())
					     : uiString::emptyString()),
				     mNoDlgTitle,
				     mODHelpKey(mWellDispPropDlgHelpID) )
		     .savebutton(true)
		     .savechecked(false)
		     .modal(true))
	, applyAllReq(this)
	, savedefault_(false)
	, is2ddisplay_(is2d)
{
    wd_ = Well::MGR().get( wd->multiID(),
			   Well::LoadReqs( Well::LogInfos,
					   Well::Mrkrs,
					   is2d ? Well::DispProps2D :
						  Well::DispProps3D ) );
    init();
}


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, const MultiID& wid,
				      bool is2d )
	: uiDialog(p,uiDialog::Setup(tr("Display properties of: %1")
	    .arg(toUiString(IOM().nameOf(wid))),
	    mNoDlgTitle,
	    mODHelpKey(mWellDispPropDlgHelpID) )
	    .savebutton(true)
	    .savechecked(false)
	    .modal(true))
	, applyAllReq(this)
	, savedefault_(false)
	, is2ddisplay_(is2d)
{
    wd_ = Well::MGR().get( wid, Well::LoadReqs( Well::LogInfos,
						Well::Mrkrs,
						is2d ? Well::DispProps2D :
						       Well::DispProps3D ) );
    init();
}


void uiWellDispPropDlg::init()
{
    bool is2d = is2ddisplay_;

    setCtrlStyle( OkAndCancel );
    setOkText( uiStrings::sSave() );
    setCancelText( uiStrings::sClose() );

    Well::DisplayProperties& props = wd_->displayProperties( is2ddisplay_ );

    ts_ = new uiTabStack( this, "Well display properties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(),"Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Center log properties" );
    tgs += new uiGroup( ts_->tabGroup(),"Right log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );
    if ( !is2d )
	tgs += new uiGroup( ts_->tabGroup(), "Track properties" );

    uiWellLogDispProperties* wlp1 = new uiWellLogDispProperties( tgs[0],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->left_, &(wd_->logs()) );
    uiWellLogDispProperties* wlp2 = new uiWellLogDispProperties( tgs[1],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->center_, &(wd_->logs()) );
    uiWellLogDispProperties* wlp3 = new uiWellLogDispProperties( tgs[2],
	uiWellDispProperties::Setup( tr("Line thickness"), tr("Line color"))
	.onlyfor2ddisplay(is2d), props.logs_[0]->right_, &(wd_->logs()) );

    propflds_ += wlp1;
    propflds_ += wlp2;
    propflds_ += wlp3;

    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd_->markers().getColors( markercols );

    uiWellDispProperties::Setup propsu =
	uiWellDispProperties::Setup(tr("Marker size"),tr("Marker color"))
	.onlyfor2ddisplay(is2d);
    uiWellMarkersDispProperties* wellprops = new uiWellMarkersDispProperties(
	tgs[3], propsu, props.markers_, markernms );
    wellprops->setAllMarkerNames( markernms, markercols );
    propflds_ += wellprops;

    if ( !is2d )
	propflds_ += new uiWellTrackDispProperties( tgs[4],
			    uiWellDispProperties::Setup(), props.track_ );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mAttachCB( propflds_[idx]->propChanged, uiWellDispPropDlg::propChg );
	if ( sKey::Log() == propflds_[idx]->props().subjectName() )
	{
	    if ( idx==0 )
		ts_->addTab( tgs[idx], is2d ? tr("Log 1")
					    : tr("Left Log") );
	    else if ( idx==1 )
		ts_->addTab( tgs[idx], tr("Center Log") );
	    else if ( idx==2 )
		ts_->addTab( tgs[idx], is2d ? tr("Log 2")
					    : tr("Right Log") );
	}
	else
	    ts_->addTab( tgs[idx], toUiString(
				      propflds_[idx]->props().subjectName()) );
    }

    uiPushButton* applbut = new uiPushButton( this, tr("Apply to all wells"),
			mCB(this,uiWellDispPropDlg,applyAllPush), true );
    applbut->attach( centeredBelow, ts_ );

    ts_->selChange().notify( mCB(this,uiWellDispPropDlg,tabSel) );

    wd_->ref();
    setWDNotifiers( true );

    tabSel( 0 );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    detachAllNotifiers();
    wd_->unRef();
}


void uiWellDispPropDlg::tabSel(CallBacker*)
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	int curpageid = ts_->currentPageId();
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
	mAttachCB( mDispNot, uiWellDispPropDlg::wdChg );
    else
	mDetachCB( mDispNot, uiWellDispPropDlg::wdChg );

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
    BufferStringSet markernms;
    wd_->markers().getNames( markernms );
    TypeSet<Color> markercols;
    wd_->markers().getColors( markercols );

    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(uiWellMarkersDispProperties*,mrkrfld,propflds_[idx])
	if ( !mrkrfld )
	    continue;

	mrkrfld->setAllMarkerNames( markernms, markercols );
	return;
    }
}


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( mDispNot );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    getFromScreen();
    mDispNot.trigger();
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


bool uiWellDispPropDlg::acceptOK( CallBacker* )
{
    savedefault_ = saveButtonChecked();
    return true;
}


//uiMultiWellDispPropDlg
uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p,
						ObjectSet<Well::Data>& wds,
						bool is2ddisplay )
	: uiWellDispPropDlg(p,wds[0],is2ddisplay)
	, wds_(wds)
	, wellselfld_(0)
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx< wds_.size(); idx++ )
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
    if ( !wd_ ) return;
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
	    if ( !prop.logs_.isEmpty() )
		logfld->resetProps( first ? prop.logs_[logidx]->left_
					  :	prop.logs_[logidx]->right_ );
	    first = false;
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track_ );
	else if ( mrkfld )
	{
	    BufferStringSet markernms;
	    TypeSet<Color> markercols;
	    wd_->markers().getNames( markernms );
	    wd_->markers().getColors( markercols );

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

    uiWellDispPropDlg::setWDNotifiers( true );
    resetProps( 0 );
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
