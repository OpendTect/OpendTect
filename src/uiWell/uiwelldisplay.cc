/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplay.cc,v 1.5 2010-11-01 14:45:30 cvsbruno Exp $";

#include "uiwelldisplay.h"

#include "welldata.h"
#include "uistatusbar.h"
#include "uiwelldisplaycontrol.h"
#include "uiwellinfopanels.h"
#include "uiwelllogdisplay.h"
#include "welllogset.h"
#include "uiwellstratdisplay.h"

#include  "uimainwin.h"

uiWellDisplay::uiWellDisplay( uiParent* p, const Well::Data& w, const Setup& s )
    : uiGroup(p,w.name())
    , wd_(w)
    , setup_(s)	    
    , zrg_(mUdf(float),0)
    , dispzinft_(SI().depthsInFeetByDefault())
    , zistime_(w.haveD2TModel()) 	       
    , control_(0)		    
{
    stratdisp_ = new uiWellStratDisplay( this );

    if ( s.nrlogdisplay_ <= 0 ) return;
    if ( s.nobackground_ )
    {
	setNoBackGround();
	if ( stratdisp_ )
	    stratdisp_->setNoBackGround();
    }

    for ( int idx=0; idx<s.nrlogdisplay_; idx++ )
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
    if ( stratdisp_ )
	stratdisp_->display( s.displaystrat_ );

    if ( s.displaystrat_ && !s.isstratbelow_ )
	stratdisp_->attach( rightOf, logdisps_[logdisps_.size()-1] );

    setHSpacing( 0 );
    setStretch( 2, 2 );
    setInitialSize();

    resetDahData();
    resetWDDisplayProperties();
}


uiWellDisplay::~uiWellDisplay()
{
    if ( control_ )
	{ delete control_; control_ = 0; }
}


void uiWellDisplay::setInitialSize()
{
    int initwidth = setup_.preflogsz_.width();
    int initheight = setup_.preflogsz_.height();

    int newwidth = logdisps_.size()*initwidth;
    if ( stratdisp_ && setup_.displaystrat_ && !setup_.isstratbelow_ )
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


void uiWellDisplay::setNewWidth( int width )
{
    int nrelems  = logdisps_.size();
    if ( stratdisp_ && !setup_.isstratbelow_ ) nrelems += 1;
    if ( !nrelems ) return;
    const int singlewidth = (int)(width/(float)nrelems );
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	logdisps_[idx]->setMinimumWidth( singlewidth );
	logdisps_[idx]->setStretch( 2, 2 );
    }
    if ( stratdisp_ )
    {
	stratdisp_->setMinimumWidth( singlewidth );
	stratdisp_->setStretch( 2, 2 );
    }
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


void uiWellDisplay::resetDahData()
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


void uiWellDisplay::resetWDDisplayProperties()
{
    const Well::DisplayProperties& props = wd_.displayProperties();
    if ( logdisps_.size() )
	setDisplayProperties( 0, wd_.displayProperties() );
}


void uiWellDisplay::setDisplayProperties( int logidx, 
					  const Well::DisplayProperties& dpp ) 
{
    //only for logs, TODO markes  
    uiWellLogDisplay::LogData& ld1 = logdisps_[logidx]->logData(true);
    uiWellLogDisplay::LogData& ld2 = logdisps_[logidx]->logData(false);

    const Well::DisplayProperties::Log& logpp1 = dpp.left_;
    const Well::DisplayProperties::Log& logpp2 = dpp.right_;

    const Well::Log* l1 = wd_.logs().getLog( logpp1.name_ );
    const Well::Log* l2 = wd_.logs().getLog( logpp2.name_ );

    ld1.wl_ = l1;			ld2.wl_ = l2;
    ld1.xrev_ = true;			ld2.xrev_ = false;
    ld1.disp_ = logpp1;			ld2.disp_ = logpp2;

    logdisps_[logidx]->dataChanged();
}


void uiWellDisplay::getDisplayProperties( 
			ObjectSet<Well::DisplayProperties>& props ) const 
{
    for ( int logidx=0; logidx<logdisps_.size(); logidx++ )
    {
	Well::DisplayProperties* pp = new Well::DisplayProperties();
	const uiWellLogDisplay* ld = logdisps_[logidx];
	const uiWellLogDisplay::LogData& ld1 = ld->logData(true);
	const uiWellLogDisplay::LogData& ld2 = ld->logData(false);
	pp->left_ = ld1.disp_; 		pp->right_ = ld2.disp_;
	props += pp;
    }
}


void uiWellDisplay::applyWDChanged()
{
    resetDahData();
}


void uiWellDisplay::setDragMode(uiGraphicsViewBase::ODDragMode& mode )
{
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	logdisps_[idx]->setDragMode( mode);
    }
    if ( stratdisp_ )
	stratdisp_->setDragMode( mode );
}



uiWellDisplayWin::uiWellDisplayWin(uiParent* p ,Well::Data& wd)
    : uiMainWin(p,wd.name())
    , wd_(wd)  
    , wellinfo_(0)
{
    setStretch( 2, 2 );
    uiWellDisplay::Setup su;

    setPrefWidth( 60 );
    setPrefHeight( 600 );

    welldisp_ = new uiWellDisplay( this, wd, su );
    welldisp_->control()->posChanged.notify(
				    mCB(this,uiWellDisplayWin,dispInfoMsg) );
    wd_.tobedeleted.notify( mCB(this,uiWellDisplayWin,closeWin) );
    wd_.dispparschanged.notify( mCB(this,uiWellDisplayWin,updateProperties) );

    finaliseDone.notify(mCB(this,uiWellDisplayWin,mkInfoPanel));
}


void uiWellDisplayWin::closeWin( CallBacker* )
{ close(); }


void uiWellDisplayWin::dispInfoMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,mesg,cb);
    statusBar()->message( mesg.buf() );
}


void uiWellDisplayWin::mkInfoPanel( CallBacker* )
{
    wellinfo_ = new uiWellDispInfoPanel( this, *welldisp_ );
    wellinfo_->setInitialSize( uiSize(50,100) );
    wellinfo_->attach( alignedAbove, welldisp_ );
}


void uiWellDisplayWin::updateProperties( CallBacker* )
{
    welldisp_->setDisplayProperties( 0, wd_.displayProperties() );
    wellinfo_->resetPropsFromWellDisp();
}

