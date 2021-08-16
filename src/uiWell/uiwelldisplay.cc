/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldisplay.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwelllogdisplay.h"
#include "uiwellstratdisplay.h"

#include  "uimainwin.h"

uiWellDisplay::uiWellDisplay( uiParent* p, Well::Data& w,
			      const Setup& s )
    : uiGroup(p,w.name())
    , wd_(w)
    , setup_(s)
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeet())
    , zistime_(w.haveD2TModel() && SI().zIsTime())
    , use3ddisp_(s.takedisplayfrom3d_)
    , control_(0)
    , stratdisp_(0)
{
    const Well::DisplayProperties& disp = wd_.displayProperties( !use3ddisp_ );

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
	auto* wld = new uiWellLogDisplay( this, wlsu );
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
    if ( s.nobackground_ )
    {
	setNoBackGround();
	if ( stratdisp_ )
	    stratdisp_->setNoBackGround();
    }

    setHSpacing( 0 );
    setStretch( 2, 2 );

    setDahData();
    setDisplayProperties();

    mAttachCB( wd_.d2tchanged, uiWellDisplay::applyWDChanges );
    mAttachCB( wd_.markerschanged, uiWellDisplay::applyWDChanges );
    if ( use3ddisp_ )
	mAttachCB( wd_.disp3dparschanged, uiWellDisplay::applyWDChanges );
    else
	mAttachCB( wd_.disp2dparschanged, uiWellDisplay::applyWDChanges );
}


uiWellDisplay::~uiWellDisplay()
{
    detachAllNotifiers();
    delete control_;
    deepErase( logdisps_ );
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
    uiWellDahDisplay::Data data( &wd_ );
    data.zrg_ = zrg_;
    data.dispzinft_ = dispzinft_;
    data.zistime_ = zistime_;

    for ( int idx=0; idx<logdisps_.size(); idx++ )
	logdisps_[idx]->setData( data );

    if ( stratdisp_ )
	stratdisp_->setData( data );
}


void uiWellDisplay::setDisplayProperties()
{
    const Well::DisplayProperties& dpp = wd_.displayProperties( !use3ddisp_ );

    for ( int idx=0; idx<logdisps_.size(); idx ++ )
    {
	uiWellLogDisplay::LogData& ld1 = logdisps_[idx]->logData(true);
	uiWellLogDisplay::LogData& ld2 = logdisps_[idx]->logData(false);

	if ( !dpp.logs_.validIdx( idx ) ) continue;
	const Well::DisplayProperties::LogCouple* lc = dpp.logs_[idx];
	const Well::DisplayProperties::Log& lp1 = lc->left_.name_!="None" ?
								lc->left_ :
								lc->center();
	const Well::DisplayProperties::Log& lp2 = lc->right_;

	const Well::Log* l1 = wd_.getLog( lp1.name_ );
	const Well::Log* l2 = wd_.getLog( lp2.name_ );

	ld1.setLog( l1 );			ld2.setLog( l2 );
	ld1.xrev_ = false;			ld2.xrev_ = false;
	ld1.disp_ = lp1;			ld2.disp_ = lp2;

	logdisps_[idx]->markerDisp() = dpp.markers_;
	logdisps_[idx]->dataChanged();
    }
}


void uiWellDisplay::applyWDChanges( CallBacker* )
{
    setDahData();
    setDisplayProperties();
}


uiWellDisplayWin::uiWellDisplayWin(uiParent* p, Well::Data& wd )
    : uiMainWin(p,toUiString(wd.name()))
    , wd_(wd)
{
    setStretch( 2, 2 );
    uiWellDisplay::Setup su; su.takedisplayfrom3d_ = true;

    setPrefWidth( 60 );
    setPrefHeight( 600 );

    welldisp_ = new uiWellDisplay( this, wd, su );
    welldisp_->setPrefWidth( 60 );
    welldisp_->setPrefHeight( 600 );
    welldisp_->control()->posChanged.notify(
				    mCB(this,uiWellDisplayWin,dispInfoMsg) );
    wd_.ref();
}


uiWellDisplayWin::uiWellDisplayWin(uiParent* p, const MultiID& mid )
    : uiMainWin(p)
    , wd_(*new Well::Data()) //NOT used!!
{
    auto wd = Well::MGR().get( mid, Well::LoadReqs( Well::DispProps2D,
						    Well::LogInfos ) );
    if ( !wd )
	return;
    setCaption( toUiString( wd->name() ) );
    setStretch( 2, 2 );
    uiWellDisplay::Setup su; su.takedisplayfrom3d_ = true;

    setPrefWidth( 60 );
    setPrefHeight( 600 );

    welldisp_ = new uiWellDisplay( this, *wd, su );
    welldisp_->setPrefWidth( 60 );
    welldisp_->setPrefHeight( 600 );
    welldisp_->control()->posChanged.notify(
				    mCB(this,uiWellDisplayWin,dispInfoMsg) );
}


uiWellDisplayWin::~uiWellDisplayWin()
{
    detachAllNotifiers();
    wd_.unRef();
}


void uiWellDisplayWin::closeWin( CallBacker* )
{
    delete welldisp_;
    welldisp_ = 0;
    wd_.unRef();
    close();
}


void uiWellDisplayWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mToUiStringTodo(mesg.buf()) );
}
