/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiwelldisplay.h"

#include "welldata.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwelllogdisplay.h"
#include "uiwellstratdisplay.h"

#include  "uimainwin.h"

uiWellDisplay::uiWellDisplay( uiParent* p, Well::Data& w, const Setup& s )
    : uiGroup(p,w.name())
    , wd_(w)
    , setup_(s)	    
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeetByDefault())
    , zistime_(w.haveD2TModel() && SI().zIsTime())
    , is3ddisp_(s.takedisplayfrom3d_)				
    , control_(0)
    , stratdisp_(0) 
{
    Well::DisplayProperties& disp = wd_.displayProperties( !is3ddisp_ );
    if ( disp.markers_.selmarkernms_.isEmpty() ) 
    {
	for ( int idx=0; idx<wd_.markers().size(); idx++ )
	{
	    if ( !wd_.markers()[idx] ) continue;
	    const char* mrkrnm = wd_.markers()[idx]->name();
	    disp.markers_.selmarkernms_.addIfNew( mrkrnm );
	}
    }

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
    if ( s.nobackground_ )
    {
	setNoBackGround();
	if ( stratdisp_ )
	    stratdisp_->setNoBackGround();
    }

    setHSpacing( 0 );
    setVSpacing( 0 );
    setStretch( 2, 2 );
    setInitialSize();

    setDahData();
    setDisplayProperties();

    wd_.d2tchanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
    wd_.markerschanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
    if ( is3ddisp_ )
	wd_.disp3dparschanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
    else
	wd_.disp2dparschanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
}


uiWellDisplay::~uiWellDisplay()
{
    wd_.d2tchanged.remove(mCB(this,uiWellDisplay,applyWDChanges) );
    wd_.markerschanged.remove(mCB(this,uiWellDisplay,applyWDChanges) );
    if ( is3ddisp_ )
	wd_.disp3dparschanged.remove(mCB(this,uiWellDisplay,applyWDChanges) );
    else
	wd_.disp2dparschanged.remove(mCB(this,uiWellDisplay,applyWDChanges) );
    if ( control_ )
	{ delete control_; control_ = 0; }
}


void uiWellDisplay::setInitialSize()
{
    int initwidth = setup_.preflogsz_.width();
    int initheight = setup_.preflogsz_.height();

    int newwidth = logdisps_.size()*initwidth;
    if ( stratdisp_ && stratdisp_ )
    {
	newwidth += initwidth;
    }

    size_ = uiSize( newwidth, initheight ); 
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
    uiWellDahDisplay::Data data;
    data.zrg_ = zrg_;
    data.dispzinft_ = dispzinft_;
    data.wd_ = &wd_;

    for ( int idx=0; idx<logdisps_.size(); idx++ )
	logdisps_[idx]->setData( data );

    if ( stratdisp_ )
	stratdisp_->setData( data );
}


void uiWellDisplay::setDisplayProperties() 
{
    const Well::DisplayProperties& dpp = wd_.displayProperties( !is3ddisp_ );

    for ( int idx=0; idx<logdisps_.size(); idx ++ )
    {
	uiWellLogDisplay::LogData& ld1 = logdisps_[idx]->logData(true);
	uiWellLogDisplay::LogData& ld2 = logdisps_[idx]->logData(false);

	if ( !dpp.logs_.validIdx( idx ) ) continue;
	const Well::DisplayProperties::Log& lp1 = dpp.logs_[idx]->left_;
	const Well::DisplayProperties::Log& lp2 = dpp.logs_[idx]->right_;

	const Well::Log* l1 = wd_.logs().getLog( lp1.name_ );
	const Well::Log* l2 = wd_.logs().getLog( lp2.name_ );

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
    : uiMainWin(p,wd.name())
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
    wd_.tobedeleted.notify( mCB(this,uiWellDisplayWin,closeWin) );
}


void uiWellDisplayWin::closeWin( CallBacker* )
{ close(); }


void uiWellDisplayWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mesg.buf() );
}

