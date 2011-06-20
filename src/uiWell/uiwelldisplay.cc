/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplay.cc,v 1.12 2011-06-20 12:21:38 cvsbruno Exp $";

#include "uiwelldisplay.h"

#include "welldata.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwellinfopanels.h"
#include "uiwelllogdisplay.h"
#include "welllogset.h"
#include "uiwellstratdisplay.h"

#include  "uimainwin.h"

uiWellDisplay::uiWellDisplay( uiParent* p, Well::Data& w, const Setup& s )
    : uiGroup(p,w.name())
    , wd_(w)
    , setup_(s)	    
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeetByDefault())
    , zistime_(w.haveD2TModel())
    , is3ddisp_(s.takedisplayfrom3d_)				
    , control_(0)
    , stratdisp_(0) 
{
    wd_.d2tchanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
    if ( is3ddisp_ )
	wd_.disp3dparschanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );
    else
	wd_.disp2dparschanged.notify(mCB(this,uiWellDisplay,applyWDChanges) );

    const Well::DisplayProperties& disp = wd_.displayProperties( !is3ddisp_ );

    for ( int idx=0; idx<disp.logs_.size(); idx++ )
    {
	uiWellLogDisplay::Setup wlsu; 
	wlsu.noyannot_ = s.noyannot_;
	wlsu.noxannot_ = s.noxannot_;
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
		control_->addLogDisplay( *wld );
	}
    }
    if ( disp.displaystrat_ )
    {
	stratdisp_ = new uiWellStratDisplay( this );
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
    setInitialSize();

    setDahData();
    setDisplayProperties();
}


uiWellDisplay::~uiWellDisplay()
{
    wd_.d2tchanged.remove(mCB(this,uiWellDisplay,applyWDChanges) );
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
	stratdisp_->setPrefWidth( initwidth );
	stratdisp_->setPrefHeight( initheight );
	newwidth += initwidth;
    }
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	logdisps_[idx]->setPrefWidth( initwidth );
	logdisps_[idx]->setPrefHeight( initheight );
    }
    setPrefWidth( newwidth ); 
    setPrefHeight( initheight );

    size_ = uiSize( newwidth, initheight ); 
}


void uiWellDisplay::setControl( uiWellDisplayControl& ctrl )
{
    if ( control_ ) delete control_;
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	ctrl.addLogDisplay( *logdisps_[idx] );
    }
    control_ = &ctrl;
}


void uiWellDisplay::setDahData()
{
    uiWellDahDisplay::Data data;
    data.zrg_ = zrg_;
    data.dispzinft_ = dispzinft_;
    data.zistime_ = zistime_;
    data.d2tm_ = wd_.d2TModel();
    data.markers_ = (ObjectSet<Well::Marker>*)&wd_.markers();

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

	const Well::DisplayProperties::Log& lp1 = dpp.logs_[idx]->left_;
	const Well::DisplayProperties::Log& lp2 = dpp.logs_[idx]->right_;

	const Well::Log* l1 = wd_.logs().getLog( lp1.name_ );
	const Well::Log* l2 = wd_.logs().getLog( lp2.name_ );

	ld1.wl_ = l1;				ld2.wl_ = l2;
	ld1.xrev_ = false;			ld2.xrev_ = false;
	ld1.disp_ = lp1;			ld2.disp_ = lp2;

	logdisps_[idx]->dataChanged();
    }
}


void uiWellDisplay::applyWDChanges( CallBacker* )
{
    setDahData();
    setDisplayProperties();
}


int uiWellDisplay::nrLogDisplays() const
{ return wd_.displayProperties( !is3ddisp_ ).logs_.size(); }



uiWellDisplayWin::uiWellDisplayWin(uiParent* p, Well::Data& wd )
    : uiMainWin(p,wd.name())
    , wd_(wd)  
{
    setStretch( 2, 2 );
    uiWellDisplay::Setup su; su.takedisplayfrom3d_ = true;

    setPrefWidth( 60 );
    setPrefHeight( 600 );

    welldisp_ = new uiWellDisplay( this, wd, su );
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

