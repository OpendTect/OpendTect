/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/

#include "uiwelldisplay.h"

#include "wellmanager.h"
#include "welllogset.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwelllogdisplay.h"
#include "uiwellstratdisplay.h"
#include "uistrings.h"


uiWellDisplay::uiWellDisplay( uiParent* p, const DBKey& id, const Setup& s )
    : uiGroup(p,"Well display")
    , setup_(s)
{
    wd_ = Well::MGR().fetch( id );
    init( s );
}


void uiWellDisplay::init( const Setup& s )
{
    zrg_ = Interval<float>( mUdf(float), 0 );
    dispzinft_ = SI().depthsInFeet();
    zistime_ = SI().zIsTime();
    use3ddisp_ = s.takedisplayfrom3d_;
    control_ = 0; stratdisp_ = 0;

    if ( !wd_ )
    {
	// show this fact to the user?
    }
    else
    {
	zistime_ = wd_->haveD2TModel() && SI().zIsTime();

	const Well::DisplayProperties2D& disp2d
			    = wd_->displayProperties2d();

	MonitorLock mldisp( disp2d );
	const int nrlogpanels = disp2d.nrPanels();
	for ( int idx=0; idx<nrlogpanels; idx++ )
	{
	    uiWellLogDisplay::Setup wlsu;
	    wlsu.noyannot_ = s.noyannot_;
	    wlsu.noxannot_ = s.noxannot_;
	    wlsu.xannotinpercents_ = s.xaxisinpercents_;
	    if ( s.nologborder_ )
	    {
		wlsu.border_ = uiBorder(0);
		wlsu.annotinside_ = true;
	    }
	    uiWellLogDisplay* wld = new uiWellLogDisplay( this, wlsu,
						*disp2d.getLogPanel(idx) );
	    mAttachCB( disp2d.getLogPanel(idx)->objectChanged(),
		       uiWellDisplay::wdChgCB );

	    logdisps_ += wld;
	    if ( s.nobackground_ )
		wld->setNoBackGround();
	    if ( idx )
		wld->attach( rightOf, logdisps_[idx-1] );

	    if ( s.withcontrol_ )
	    {
		if ( !control_ )
		    control_ = new uiWellDisplayControl( *wld );
		else
		    control_->addDahDisplay( *wld );
	    }
	}

	if ( disp2d.displayStrat() )
	{
	    stratdisp_ = new uiWellStratDisplay( this );
	    if ( !logdisps_.isEmpty() )
		stratdisp_->attach( rightOf, logdisps_[logdisps_.size()-1] );
	}
	mldisp.unlockNow();

	mAttachCB( wd_->track().objectChanged(), uiWellDisplay::wdChgCB );
	mAttachCB( wd_->d2TModel().objectChanged(), uiWellDisplay::wdChgCB );
	mAttachCB( wd_->displayProperties(!use3ddisp_).objectChanged(),
		   uiWellDisplay::wdChgCB );
	mAttachCB( wd_->displayProperties(true).markers().objectChanged(),
		   uiWellDisplay::wdChgCB );
    }

    if ( s.nobackground_ )
    {
	setNoBackGround();
	if ( stratdisp_ )
	    stratdisp_->setNoBackGround();
    }

    setHSpacing( 0 );
    setStretch( 2, 2 );

    updateDisplayFromWellData();
}


uiWellDisplay::~uiWellDisplay()
{
    detachAllNotifiers();
    delete control_;
}


void uiWellDisplay::setControl( uiWellDisplayControl& ctrl )
{
    if ( control_ ) delete control_;
    for ( int idx=0; idx<logdisps_.size(); idx++ )
	ctrl.addDahDisplay( *logdisps_[idx] );
    control_ = &ctrl;
}


void uiWellDisplay::setDahData()
{
    uiWellDahDisplay::Data data( wd_ );
    data.zrg_ = zrg_;
    data.dispzinft_ = dispzinft_;

    for ( int idx=0; idx<logdisps_.size(); idx++ )
	logdisps_[idx]->setData( data );

    if ( stratdisp_ )
	stratdisp_->setData( data );
}


void uiWellDisplay::setDisplayProperties()
{
    if ( !wd_ )
	{ clearLogDisplay(); return; }

    const Well::DisplayProperties2D& disp2d = wd_->displayProperties2d();
    MonitorLock mldisp( disp2d );

    for ( int idx=0; idx<logdisps_.size(); idx ++ )
    {
	const Well::DisplayProperties2D::LogPanelProps* panel =
						disp2d.getLogPanel( idx );
	if ( !panel )
	    continue;

	uiWellLogDisplay* wldisp = logdisps_.get( idx );
	for ( int ldidx=0; ldidx<panel->logs_.size(); ldidx++ )
	{
	    const Well::LogDispProps& lp = *panel->getLog( ldidx );
	    const Well::Log* log = wd_->logs().getLogByName( lp.logName() );
	    uiWellLogDisplay::LogData& logdata = wldisp->logData( ldidx );

	    logdata.setLog( log );
	    logdata.xrev_ = false;
	    logdata.disp_ = lp;

	    wldisp->markerDisp() = disp2d.markers();
	    wldisp->dataChanged();
	}
    }
}


void uiWellDisplay::updateDisplayFromWellData()
{
    setDahData();
    setDisplayProperties();
}


void uiWellDisplay::clearLogDisplay()
{
    int ldsisp = logdisps_.size();
    for ( int ldidx=0; ldidx<ldsisp; ldidx ++ )
    {
	for ( int idx=0; idx<logdisps_[ldidx]->nrLogs(); idx++ )
	    logdisps_[ldidx]->logData(idx).setLog(0);
    }
}


void uiWellDisplay::wdChgCB( CallBacker* )
{
    //get ChangeType . based on that update the log
    updateDisplayFromWellData();
}


void uiWellDisplay::wellReloadCB( CallBacker* )
{
    clearLogDisplay();
}


uiWellDisplayWin::uiWellDisplayWin( uiParent* p, const DBKey& id,
				    bool withcontrol )
    : uiMainWin(p,getWinTitle(id,withcontrol))
{
    setStretch( 2, 2 );
    setPrefWidth( 60 ); setPrefHeight( 600 );

    uiWellDisplay::Setup su;
    su.takedisplayfrom3d( true ).withcontrol( withcontrol );
    welldisp_ = new uiWellDisplay( this, id, su );
    welldisp_->setPrefWidth( 60 );
    welldisp_->setPrefHeight( 600 );
    welldisp_->control()->posChanged.notify(
					mCB(this,uiWellDisplayWin,posChgCB) );
}


uiWellDisplayWin::~uiWellDisplayWin()
{
    detachAllNotifiers();
}


uiString uiWellDisplayWin::getWinTitle( const DBKey& id, bool foredit )
{
    return toUiString( "%1 '%2'" )
		.arg( foredit ? uiStrings::sEdit() : uiStrings::sDisplay() )
		.arg( id.name() );
}


void uiWellDisplayWin::posChgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( toUiString(mesg.buf()) );
}
