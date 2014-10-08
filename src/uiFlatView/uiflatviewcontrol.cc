/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatviewcontrol.h"
#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"
#include "uigraphicsscene.h"
#include "uiworld2ui.h"


uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, uiParent* p, 
				bool rub, bool withhanddrag )
    : uiGroup(p ? p : vwr.attachObj()->parent(),"Flat viewer control")
    , zoommgr_(*new FlatView::ZoomMgr)
    , haverubber_(rub)
    , withhanddrag_(withhanddrag)
    , propdlg_(0)
    , infoChanged(this)
    , viewerAdded(this)
    , zoomChanged(this)
{
    setBorder( 0 );
    addViewer( vwr );
    if ( vwr.attachObj()->parent() )
	mAttachCB( vwr.attachObj()->parent()->postFinalise(),
			uiFlatViewControl::onFinalise );
    mAttachCB( viewerAdded, uiFlatViewControl::vwrAdded );
}


uiFlatViewControl::~uiFlatViewControl()
{
    detachAllNotifiers();
    delete &zoommgr_;
    delete propdlg_;
    propdlg_ = 0;
}


void uiFlatViewControl::addViewer( uiFlatViewer& vwr )
{
    vwrs_ += &vwr;
    vwr.control_ = this;
    zoommgr_.setNrViewers( vwrs_.size() );
    mAttachCB( vwr.dataChanged, uiFlatViewControl::dataChangeCB );

    if ( haverubber_ )
	mAttachCB(vwr.rgbCanvas().rubberBandUsed,uiFlatViewControl::rubBandCB);

    MouseEventHandler& mevh = mouseEventHandler( vwrs_.size()-1, true );
    mAttachCB( mevh.movement, uiFlatViewControl::mouseMoveCB );
    mAttachCB( mevh.buttonReleased, uiFlatViewControl::usrClickCB );
    mAttachCB( vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
		uiFlatViewControl::keyPressCB );

    viewerAdded.trigger();
}


void uiFlatViewControl::removeViewer( uiFlatViewer& vwr )
{
    vwrs_ -= &vwr;
    zoommgr_.setNrViewers( !vwrs_.size() ? 1 : vwrs_.size() );
    mDetachCB( vwr.dataChanged, uiFlatViewControl::dataChangeCB );

    uiGraphicsView& cnvs = vwr.rgbCanvas();
    if ( haverubber_ )
	mDetachCB( cnvs.rubberBandUsed, uiFlatViewControl::rubBandCB );

    MouseEventHandler& mevh = cnvs.scene().getMouseEventHandler();
    mDetachCB( mevh.movement, uiFlatViewControl::mouseMoveCB );
    mDetachCB( mevh.buttonReleased, uiFlatViewControl::usrClickCB );
    mDetachCB( vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
		uiFlatViewControl::keyPressCB );
}


TypeSet<uiWorldRect> uiFlatViewControl::getBoundingBoxes() const
{
    TypeSet<uiWorldRect> wrs;

    for ( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
	wrs += vwrs_[ivwr]->boundingBox();

    return wrs;
}


void uiFlatViewControl::onFinalise( CallBacker* )
{
    const bool canreuse = zoommgr_.current().width() > 10
			 && canReUseZoomSettings( vwrs_[0]->curView().centre(),
						  zoommgr_.current() );
    if ( !canreuse )
    {
	zoommgr_.init( getBoundingBoxes() );
    }

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !canreuse )
	    vwrs_[idx]->setViewToBoundingBox();
	else
    	    vwrs_[idx]->setView( vwrs_[idx]->curView() );
    }

    finalPrepare();
}


void uiFlatViewControl::dataChangeCB( CallBacker* cb )
{
    zoommgr_.reInit( getBoundingBoxes() );
    zoomChanged.trigger();
}


bool uiFlatViewControl::havePan(
	Geom::Point2D<double> oldcentre, Geom::Point2D<double> newcentre,
	Geom::Size2D<double> sz )
{
    const Geom::Point2D<double> eps( sz.width()*1e-6, sz.height()*1e-6 );
    return !mIsZero(oldcentre.x-newcentre.x,eps.x)
	|| !mIsZero(oldcentre.y-newcentre.y,eps.y);
}


bool uiFlatViewControl::haveZoom( Geom::Size2D<double> oldsz,
				  Geom::Size2D<double> newsz )
{
    const Geom::Point2D<double> eps( oldsz.width()*1e-6, oldsz.height()*1e-6 );
    return !mIsZero(oldsz.width()-newsz.width(),eps.x)
	|| !mIsZero(oldsz.height()-newsz.height(),eps.y);
}


uiTabStackDlg* uiFlatViewControl::propDialog()
{ return propdlg_; }


void uiFlatViewControl::setNewView( Geom::Point2D<double> mousepos,
				    Geom::Size2D<double> sz )
{
    uiFlatViewer* vwr = vwrs_[0];
    const uiWorldRect wr = getZoomOrPanRect( mousepos, sz, vwr->curView(),
	    				     vwr->boundingBox() );
    vwr->setView( wr );
    zoommgr_.add( sz );
    zoomChanged.trigger();
}


uiWorldRect uiFlatViewControl::getZoomOrPanRect( Geom::Point2D<double> mousepos,
						 Geom::Size2D<double> newsz,
						 const uiWorldRect& view,
						 const uiWorldRect& bbox )
{
    if ( mIsZero(newsz.width(),mDefEps) || mIsZero(newsz.height(),mDefEps) )
	return view;

    uiWorldRect cv( view ); cv.sortCorners(true,false);
    uiWorldRect bb( bbox ); bb.sortCorners(true,false);

    if ( newsz.width() > bb.width() ) newsz.setWidth( bb.width() );
    if ( newsz.height() > bb.height() ) newsz.setHeight( bb.height() );

    const double lwdth = newsz.width() * (mousepos.x-cv.left())/cv.width();
    const double bhght = newsz.height() * (mousepos.y-cv.bottom())/cv.height();
    const double rwdth = newsz.width() * (cv.right()-mousepos.x)/cv.width();
    const double thght = newsz.height() * (cv.top()-mousepos.y)/cv.height();

    if ( mousepos.x - lwdth < bb.left() )      mousepos.x = bb.left() + lwdth;
    if ( mousepos.y - bhght < bb.bottom() )    mousepos.y = bb.bottom() + bhght;
    if ( mousepos.x + rwdth > bb.right() )     mousepos.x = bb.right() - rwdth;
    if ( mousepos.y + thght > bb.top() )       mousepos.y = bb.top() - thght;

    return uiWorldRect( mousepos.x - lwdth, mousepos.y + thght,
			mousepos.x + rwdth, mousepos.y - bhght );
}


void uiFlatViewControl::flip( bool hor )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::Annotation::AxisData& ad
			    = hor ? vwrs_[idx]->appearance().annot_.x1_
				  : vwrs_[idx]->appearance().annot_.x2_;
	ad.reversed_ = !ad.reversed_;
	vwrs_[idx]->handleChange( FlatView::Viewer::Annot );
    }
}


void uiFlatViewControl::rubBandCB( CallBacker* cb )
{
    uiFlatViewer* vwr = vwrs_[0];
    const uiRect* selarea = vwr->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	 (selarea->width()<5 && selarea->height()<5) )
	return;

    uiWorld2Ui w2u;
    vwr->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    Geom::Size2D<double> newsz = wr.size();

    wr = getZoomOrPanRect( wr.centre(), newsz, wr, vwr->boundingBox() );
    vwr->setView( wr );
    zoommgr_.add( newsz );
    zoomChanged.trigger();
}


void uiFlatViewControl::doPropertiesDialog( int vieweridx )
{
    uiFlatViewer& vwr = *vwrs_[vieweridx];
    BufferStringSet annots;
    const int selannot = vwr.getAnnotChoices( annots ); 

    if ( !propdlg_ || &propdlg_->viewer()!=&vwr )
    {
	delete propdlg_;
    	propdlg_ = new uiFlatViewPropDlg( 0, vwr,
				mCB(this,uiFlatViewControl,applyProperties),
				annots.size() ? &annots : 0, selannot );
    	mAttachCB( propdlg_->windowClosed, uiFlatViewControl::propDlgClosed );
    }

    propdlg_->show();
}


void uiFlatViewControl::propDlgClosed( CallBacker* )
{
    if ( propdlg_->uiResult() == 1 )
    {
	applyProperties(0);
	if ( propdlg_->saveButtonChecked() )
	    saveProperties( propdlg_->viewer() );
    }
}


void uiFlatViewControl::applyProperties( CallBacker* cb )
{
    if ( !propdlg_ ) return;

    mDynamicCastGet( uiFlatViewer*, vwr, &propdlg_->viewer() );
    if ( !vwr ) return;

    const int selannot = propdlg_->selectedAnnot();
    vwr->setAnnotChoice( selannot );
    vwr->handleChange( FlatView::Viewer::Annot |
	    	       FlatView::Viewer::DisplayPars );
    vwr->dispPropChanged.trigger();
}


void uiFlatViewControl::saveProperties( FlatView::Viewer& vwr )
{
    ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( true, true );
    vwr.storeDefaults( fdp ? fdp->category() : "General" );
}


MouseEventHandler& uiFlatViewControl::mouseEventHandler( int idx, bool ofscene )
{
    uiGraphicsView& canvas = vwrs_[idx]->rgbCanvas();
    return ofscene ? canvas.scene().getMouseEventHandler()
		   : canvas.getNavigationMouseEventHandler();
}


int uiFlatViewControl::getViewerIdx( const MouseEventHandler* meh,bool ofscene )
{
    if ( !meh ) return -1;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	const MouseEventHandler* imeh = &mouseEventHandler( idx, ofscene );
	if ( imeh==meh && imeh->hasEvent() ) return idx;
    }

    return -1;
}


void uiFlatViewControl::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const int idx = getViewerIdx( meh, true );
    if ( idx<0 ) return;
    uiWorld2Ui w2u;
    if ( !vwrs_[idx]->needStatusBarUpdate() ) return;
    vwrs_[idx]->getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( meh->event().pos() );
    vwrs_[idx]->getAuxInfo( wp, infopars_ );
    CBCapsule<IOPar> caps( infopars_, this );
    infoChanged.trigger( &caps );
}


void uiFlatViewControl::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() || meh->isHandled() )
	return;

    meh->setHandled( handleUserClick(getViewerIdx(meh,true)) );
}


bool uiFlatViewControl::canReUseZoomSettings( Geom::Point2D<double> centre,
					      Geom::Size2D<double> sz ) const
{
    //TODO: allow user to decide to reuse or not with a specific parameter
    const uiWorldRect bb( vwrs_[0]->boundingBox() );
    if ( sz.width() > bb.width() || sz.height() > bb.height() )
	return false;
    
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    Geom::Point2D<double> topleft( centre.x - hwdth, centre.y - hhght );
    Geom::Point2D<double> botright( centre.x + hwdth, centre.y + hhght );
    if ( ! ( bb.contains( topleft, 1e-6 ) && bb.contains( botright, 1e-6 ) ) )
	return false;

    return true;
}
