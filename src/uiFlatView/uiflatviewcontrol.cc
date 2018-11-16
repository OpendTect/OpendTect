/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "mouseevent.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"
#include "uigraphicsscene.h"


uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, uiParent* p, bool rub )
    : uiGroup(p ? p : vwr.attachObj()->parent(),"Flat viewer control")
    , haverubber_(rub)
    , propdlg_(0)
    , infoChanged(this)
    , viewerAdded(this)
    , zoomChanged(this)
    , rubberBandUsed(this)
    , initdone_(false)
{
    addViewer( vwr );
    if ( vwr.attachObj()->parent() )
	mAttachCB( vwr.attachObj()->parent()->postFinalise(),
			uiFlatViewControl::onFinalise );
    mAttachCB( viewerAdded, uiFlatViewControl::vwrAdded );

    // Hack to set correct zoom level at start-up.
    mAttachCB( vwr.rgbCanvas().reSize, uiFlatViewControl::initZoom );
}


uiFlatViewControl::~uiFlatViewControl()
{
    detachAllNotifiers();
    deleteAndZeroPtr( propdlg_ );
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


void uiFlatViewControl::initZoom( CallBacker* )
{
    if ( initdone_ )
	return;

    initdone_ = true;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	setViewToCustomZoomLevel( *vwrs_[idx] );
}


void uiFlatViewControl::onFinalise( CallBacker* )
{
    const bool canreuse = !zoommgr_.atStart() &&
			   canReUseZoomSettings(*vwrs_[0]);
    if ( !canreuse )
	zoommgr_.init( getBoundingBoxes() );

    finalPrepare();
}


void uiFlatViewControl::dataChangeCB( CallBacker* cb )
{
    zoommgr_.reInit( getBoundingBoxes() );
    zoomChanged.trigger();
}


void uiFlatViewControl::reInitZooms()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setViewToBoundingBox();
    zoommgr_.reInit( getBoundingBoxes() );
    zoommgr_.toStart();
    zoomChanged.trigger();
}


void uiFlatViewControl::setNewView( Geom::Point2D<double> mousepos,
				    Geom::Size2D<double> sz,
				    uiFlatViewer& vwr )
{
    uiWorldRect wr = vwr.curView();
    const uiWorldRect bb = vwr.boundingBox();
    const bool withfixedaspectratio = !vwr.updatesBitmapsOnResize();
    if ( !withfixedaspectratio )
	wr = getZoomOrPanRect( mousepos, sz, vwr.curView(), bb );
    else
    {
	if ( sz.width()<=bb.width() && sz.height()<=bb.height() )
	{
	    wr = getZoomOrPanRect( mousepos, sz, vwr.curView(), bb );
	    if ( vwr.getViewRect() != vwr.getViewRect(false) )
	    {
		const uiWorld2Ui w2ui( vwr.getViewRect(), wr );
		wr = w2ui.transform( vwr.getViewRect(false) );
		vwr.setBoundingRect( w2ui.transform(bb) );
	    }
	}
	else
	{
	    const double hwdth = sz.width()/2, hhght = sz.height()/2;
	    wr = uiWorldRect( mousepos.x_-hwdth, mousepos.y_-hhght,
			      mousepos.x_+hwdth, mousepos.y_+hhght );
	    vwr.setBoundingRect(uiWorld2Ui(vwr.getViewRect(),wr).transform(bb));
	}
	wr = getZoomOrPanRect( wr.centre(), wr.size(), wr, bb );
    }

    vwr.setView( wr );
    updateZoomManager();
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

    const double lwdth = newsz.width() * (mousepos.x_-cv.left())/cv.width();
    const double bhght = newsz.height() * (mousepos.y_-cv.bottom())/cv.height();
    const double rwdth = newsz.width() * (cv.right()-mousepos.x_)/cv.width();
    const double thght = newsz.height() * (cv.top()-mousepos.y_)/cv.height();

    if ( mousepos.x_ - lwdth < bb.left() )
	mousepos.x_ = bb.left() + lwdth;
    if ( mousepos.y_ - bhght < bb.bottom() )
	mousepos.y_ = bb.bottom() + bhght;
    if ( mousepos.x_ + rwdth > bb.right() )
	mousepos.x_ = bb.right() - rwdth;
    if ( mousepos.y_ + thght > bb.top() )
	mousepos.y_ = bb.top() - thght;

    return uiWorldRect( mousepos.x_ - lwdth, mousepos.y_ + thght,
			mousepos.x_ + rwdth, mousepos.y_ - bhght );
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

    const uiWorldRect wr = vwr->getWorld2Ui().transform( *selarea );
    setNewView( wr.centre(), wr.size(), *vwr );
    rubberBandUsed.trigger();
}


void uiFlatViewControl::updateZoomManager()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	zoommgr_.add( vwrs_[idx]->curView().size(), idx );

    zoomChanged.trigger();
}


void uiFlatViewControl::doPropertiesDialog( int vieweridx )
{
    uiFlatViewer& vwr = *vwrs_[vieweridx];
    if ( !propdlg_ || &propdlg_->viewer()!=&vwr )
    {
	delete propdlg_;
	BufferStringSet annots;
	const int selannot = vwr.getAnnotChoices( annots );
	propdlg_ = new uiFlatViewPropDlg( parent(), vwr,
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


void uiFlatViewControl::applyProperties( CallBacker* )
{
    if ( !propdlg_ ) return;

    mDynamicCastGet(uiFlatViewer*,vwr,&propdlg_->viewer());
    if ( !vwr ) return;

    const bool updateonresize = vwr->updatesBitmapsOnResize();
    vwr->updateBitmapsOnResize( true );

    const int selannot = propdlg_->selectedAnnot();
    vwr->setAnnotChoice( selannot );
    vwr->handleChange( FlatView::Viewer::Annot |
	    	       FlatView::Viewer::DisplayPars );
    vwr->dispPropChanged.trigger();

    vwr->updateBitmapsOnResize( updateonresize );
}


void uiFlatViewControl::saveProperties( FlatView::Viewer& vwr )
{
    ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true );
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
    uiFlatViewer* vwr = vwrs_[idx];
    if ( !vwr->needStatusBarUpdate() ) return;

    const Geom::Point2D<int> mousepos = meh->event().pos();
    const uiWorldPoint wp = vwr->getWorld2Ui().transform( mousepos );
    const bool isinsde = vwr->getViewRect().isInside( mousepos );
    IOPar infopar;
    if ( isinsde )
    {
	vwr->getAuxInfo( wp, infopars_ );
	infopar = infopars_;
    }

    CBCapsule<IOPar> caps( infopar, this );
    infoChanged.trigger( &caps );
}


void uiFlatViewControl::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() || meh->isHandled() )
	return;

    meh->setHandled( handleUserClick(getViewerIdx(meh,true)) );
}


bool uiFlatViewControl::canReUseZoomSettings( const uiFlatViewer& vwr ) const
{
    //TODO: allow user to decide to reuse or not with a specific parameter
    const uiWorldRect bb( vwr.boundingBox() );
    const uiWorldRect& curview = vwr.curView();
    const Geom::Size2D<double> sz = curview.size();
    if ( sz.width() > bb.width() || sz.height() > bb.height() )
	return false;
    
    return bb.contains(curview.topLeft(),1e-3) &&
	   bb.contains(curview.bottomRight(),1e-3);
}
