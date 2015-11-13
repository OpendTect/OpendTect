/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldisplay.h"

#include "ioman.h"
#include "welldata.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellman.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwelllogdisplay.h"
#include "uiwellstratdisplay.h"
#include "uistrings.h"

#include  "uimainwin.h"

/*
uiWellDisplay::uiWellDisplay( uiParent* p, Well::Data& w, const Setup& s )
    : uiGroup(p,"Well display")
    , setup_(s)	    
    , mandata_(w.data().multiID())
{
    init( s );
}
*/


uiWellDisplay::uiWellDisplay( uiParent* p, const MultiID& id, const Setup& s )
    : uiGroup(p,"Well display")
    , setup_(s)
    , mandata_(id)
{
    init( s );
}


void uiWellDisplay::init( const Setup& s )
{
    zrg_ = Interval<float>( mUdf(float), 0 );
    dispzinft_ = SI().depthsInFeet();
    zistime_ = SI().zIsTime();
    use3ddisp_ = s.takedisplayfrom3d_;
    control_ = 0; stratdisp_ = 0;

    if ( !haveWellData() )
    {
	// show this fact to the user?
    }
    else
    {
	Well::Data& wd = wellData();
	zistime_ = wd.haveD2TModel() && SI().zIsTime();

	Well::DisplayProperties& disp = wd.displayProperties( !use3ddisp_ );

	for ( int idx=0; idx<disp.logs_.size(); idx++ )
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
	    uiWellLogDisplay* wld = new uiWellLogDisplay( this, wlsu );
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

	if ( disp.displaystrat_ )
	{
	    stratdisp_ = new uiWellStratDisplay( this );
	    if ( !logdisps_.isEmpty() )
		stratdisp_->attach( rightOf, logdisps_[logdisps_.size()-1] );
	}

	mAttachCB( wd.d2tchanged, uiWellDisplay::wdChgCB );
	mAttachCB( wd.markerschanged, uiWellDisplay::wdChgCB );
	mAttachCB( wd.reloaded, uiWellDisplay::wellReloadCB);
	if ( use3ddisp_ )
	    mAttachCB( wd.disp3dparschanged, uiWellDisplay::wdChgCB );
	else
	    mAttachCB( wd.disp2dparschanged, uiWellDisplay::wdChgCB );
	wd.deleteNotify( mCB(this,uiWellDisplay,wdChgCB) );
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
    if ( haveWellData() )
    {
	Well::Data& wd = wellData();
	wd.stopDeleteNotify( *this );
    }
    delete control_;
}


bool uiWellDisplay::haveWellData() const
{
    return mandata_.isAvailable();
}


Well::Data& uiWellDisplay::wellData()
{
    return mandata_.data();
}


void uiWellDisplay::setControl( uiWellDisplayControl& ctrl )
{
    if ( control_ ) delete control_;
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	ctrl.addDahDisplay( *logdisps_[idx] );
    }
    control_ = &ctrl;
}


void uiWellDisplay::setDahData()
{
    Well::Data* wd = haveWellData() ? &wellData() : 0;
    uiWellDahDisplay::Data data( wd );
    data.zrg_ = zrg_;
    data.dispzinft_ = dispzinft_;

    for ( int idx=0; idx<logdisps_.size(); idx++ )
	logdisps_[idx]->setData( data );

    if ( stratdisp_ )
	stratdisp_->setData( data );
}


void uiWellDisplay::setDisplayProperties() 
{
    Well::Data* wd = haveWellData() ? &wellData() : 0;
    if ( !wd )
	{ clearLogDisplay(); return; }

    const Well::DisplayProperties& dpp = wd->displayProperties( !use3ddisp_ );

    for ( int idx=0; idx<logdisps_.size(); idx ++ )
    {
	uiWellLogDisplay::LogData& ld1 = logdisps_[idx]->logData(true);
	uiWellLogDisplay::LogData& ld2 = logdisps_[idx]->logData(false);

	if ( !dpp.logs_.validIdx( idx ) ) continue;
	const Well::DisplayProperties::Log& lp1 = dpp.logs_[idx]->left_;
	const Well::DisplayProperties::Log& lp2 = dpp.logs_[idx]->right_;

	const Well::Log* l1 = wd->logs().getLog( lp1.name_ );
	const Well::Log* l2 = wd->logs().getLog( lp2.name_ );

	ld1.setLog( l1 );			ld2.setLog( l2 );
	ld1.xrev_ = false;			ld2.xrev_ = false;
	ld1.disp_ = lp1;			ld2.disp_ = lp2;

	logdisps_[idx]->markerDisp() = dpp.markers_;
	logdisps_[idx]->dataChanged();
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
	logdisps_[ldidx]->logData(use3ddisp_).setLog(0);
    }
}


void uiWellDisplay::wdChgCB( CallBacker* )
{
    updateDisplayFromWellData();
}


void uiWellDisplay::wellReloadCB( CallBacker* )
{
    clearLogDisplay();
}


uiWellDisplayWin::uiWellDisplayWin( uiParent* p, const MultiID& id,
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

    if ( welldisp_->haveWellData() )
	welldisp_->deleteNotify( mCB(this,uiWellDisplayWin,wdDelCB) );
}


uiString uiWellDisplayWin::getWinTitle( const MultiID& id, bool foredit )
{
    return tr( "%1 '%2'" ).arg( foredit ? uiStrings::sEdit()
					 : uiStrings::sDisplay() )
					   .arg( IOM().nameOf( id ) );
}


void uiWellDisplayWin::wdDelCB( CallBacker* cb )
{
    if ( !welldisp_->haveWellData() )
	close();
}


void uiWellDisplayWin::posChgCB( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mToUiStringTodo(mesg.buf()) );
}
