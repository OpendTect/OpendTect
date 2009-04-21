/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID = "$Id: uiwelltiecontrolview.cc,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $";

#include "uiwelltiecontrolview.h"

#include "keyenum.h"
#include "pixmap.h"
#include "flatviewzoommgr.h"
#include "mousecursor.h"
#include "welltiedisp.h"

#include "uibutton.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"
#include "uiwelltieviewpropdlg.h"

#define mDefBut(but,fnm,cbnm,tt) \
        but = new uiToolButton( toolbar_, 0, ioPixmap(fnm), \
				mCB(this,uiWellTieControlView,cbnm) ); \
    but->setToolTip( tt ); \
    toolbar_->addObject( but );


uiWellTieControlView::uiWellTieControlView( uiParent* p,  
			    uiToolBar* toolbar, ObjectSet<uiFlatViewer>& viewer )
    : uiFlatViewControl(*viewer[0], p, true, false)
    , toolbar_(toolbar )
    , manipdrawbut_(0)
    , seisPickPosAdded(this)		      
    , synthPickPosAdded(this)
    , propdlg_(0)   			    
    , dprops_(0)					    
    , synthpicks_(0)
    , seispicks_(0)
		
{
    for ( int vwridx=1; vwridx<viewer.size(); vwridx++ )
	addViewer( *viewer[vwridx] );

    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode");
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
	vwrs_[vwridx]->setRubberBandingOn( !manip_ );

    mDefBut(zoominbut_,"zoomforward.png",zoomInCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",zoomOutCB,"Zoom out");
    //mDefBut(disppropbut_,"2ddisppars.png",doPropDlg,"Set display properties");

    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
	vwrs_[vwridx]->viewChanged.notify(
				mCB(this,uiWellTieControlView,vwChgCB) );

    new uiFlatViewThumbnail( this, *vwrs_[0] );
    
    dprops_ = new WellTieDisplayProperties();
    if (!dprops_) return;
   
    updateButtons();
}


uiWellTieControlView::~uiWellTieControlView()
{
    if ( synthpicks_ ) delete synthpicks_;
    if ( seispicks_ ) delete seispicks_;
    delete ( dprops_ );
    if ( propdlg_ ) delete propdlg_;
}


void uiWellTieControlView::updateButtons()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );
}


//replace standard rubBand because of multiple viewers
void uiWellTieControlView::rubBandCB( CallBacker* cb )
{
    BoolTypeSet iscurrentvwr;
    mDynamicCastGet( uiGraphicsView*, cber, cb )
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++) 	
	iscurrentvwr += ( cber == &vwrs_[vwridx]->rgbCanvas() );

    const uiRect* selarea = cber->getSelectedArea();
    if ( selarea->topLeft() == selarea->bottomRight() ) return;

    uiWorld2Ui w2u;
    uiWorldRect wr;
  
    //set current view
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++ )
    {
	if ( iscurrentvwr[vwridx] )
	{
	    vwrs_[ vwridx ]->getWorld2Ui( w2u );
	    wr  = w2u.transform( *selarea );
	    
	    const uiWorldRect bbox = vwrs_[vwridx]->boundingBox();

	    wr.setTopLeft( bbox.moveInside(wr.topLeft()) );
	    wr.setBottomRight( bbox.moveInside(wr.bottomRight()) );

	    Geom::Point2D<double> centre = wr.centre();
	    Geom::Size2D<double> newsz = wr.size();

	    vwrs_[vwridx]->setView( wr );
	}
    }

    uiWorldRect oldwr; 
    //set opposite views
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++ )
    {
	if ( !iscurrentvwr[vwridx] )
	{
	    oldwr = vwrs_[vwridx]->curView();
	    wr.setLeft( oldwr.left() );
	    wr.setRight( oldwr.right() );
	    vwrs_[vwridx]->setView( wr );
	}
    }
    zoommgr_.add( wr.size() );
    updateButtons();
}


void uiWellTieControlView::setView()
{
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
        vwrs_[vwridx]->setView(
			getZoomOrPanRect( vwrs_[vwridx]->curView().centre(),
			zoommgr_.current(), vwrs_[vwridx]->boundingBox() ) );
}


void uiWellTieControlView::doPropDlg( CallBacker* )
{
    if ( propdlg_ ) delete propdlg_;
    propdlg_ = new uiWellTieViewPropDlg( this, *dprops_, vwrs_ );
    propdlg_->windowClosed.notify( 
	mCB(this,uiWellTieControlView,propDlgClosed) );
    propdlg_->go();
}


void uiWellTieControlView::propDlgClosed( CallBacker* )
{
    if ( propdlg_ )
	applyProperties(0);
    setView();
}


void uiWellTieControlView::usrClickCB( CallBacker* cb )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !mouseEventHandler( idx ).hasEvent() )
	continue;

	if ( mouseEventHandler(idx).isHandled() )
	return;

	mouseEventHandler(idx).setHandled( this->handleUserClick( idx) );
    }
}


bool uiWellTieControlView::handleUserClick( const int vwridx )
{

	    const MouseEvent& ev = mouseEventHandler(vwridx).event();
	    uiWorld2Ui w2u;
	    vwrs_[vwridx]->getWorld2Ui(w2u);
	    const uiWorldPoint wp = w2u.transform( ev.pos() );
	    vwrs_[vwridx]->getAuxInfo( wp, infopars_ );
	    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
		!ev.altStatus() )
	    {
		addPickPos( vwridx,  wp.y );
		return true;
	    }
    
	    return false;
}


void uiWellTieControlView::setUserPicks( UserPicks* synp, UserPicks* seisp )
{
    synthpicks_ = synp;
    seispicks_  = seisp;
}


void uiWellTieControlView::addPickPos( const int vwridx, const float zpos )
{
    if ( !synthpicks_ && !seispicks_ ) return;
    if ( !strcmp( vwrs_[vwridx]->pack(true)->name(), "Synthetics" ) )
    {

	synthpicks_->color_ = Color::DgbColor(); 
	synthpicks_->vieweridx_ = vwridx; 
	synthpicks_->zpos_ += zpos; 
	synthPickPosAdded.trigger();
    }
    else
    {
	seispicks_->vieweridx_ = vwridx; 
	seispicks_->zpos_ += zpos;
	seisPickPosAdded.trigger();
    }
}


void uiWellTieControlView::zoomOutCB(CallBacker*)
{
    zoommgr_.back();
    setView();
    updateButtons();
}


void uiWellTieControlView::zoomInCB(CallBacker*)
{
    zoommgr_.forward();
    setView(); 
    updateButtons();
}


void uiWellTieControlView::stateCB( CallBacker* but )
{
    if ( !manipdrawbut_ ) return;

    if ( manip_ )
	manip_ = 0;
    else 
	manip_ = 1;

    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
  
    for (int vwridx=0; vwridx<vwrs_.size(); vwridx++)
	vwrs_[vwridx]->setRubberBandingOn( !manip_ );
}


void uiWellTieControlView::vwChgCB( CallBacker* )
{
    updateButtons();
}





