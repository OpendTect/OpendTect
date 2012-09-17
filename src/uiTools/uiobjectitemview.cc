/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemview.cc,v 1.23 2012/06/26 13:13:20 cvsbruno Exp $";


#include "uiobjectitemview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiparent.h"
#include "uigroup.h"

#include "draw.h"
#include "geometry.h"


uiObjectItemView::uiObjectItemView( uiParent* p )
    : uiGraphicsView(p,"Object Item Viewer")
    , viewareareset(this)
{
    uiGraphicsObjectScene* newscene = new uiGraphicsObjectScene("Object Scene");
    setScene( *newscene );
    setSceneAlignment( Alignment::HCenter );

    CallBack areacb( mCB(this,uiObjectItemView,resetViewArea) );
    reSize.notify( areacb );
    scrollBarUsed.notify( areacb );
    getMouseEventHandler().buttonReleased.notify(
	                mCB(this,uiObjectItemView,rubberBandCB) );
} 


void uiObjectItemView::enableScrollBars( bool yn )
{
    ScrollBarPolicy pol = yn ? ScrollBarAsNeeded : ScrollBarAlwaysOff;
    setScrollBarPolicy( true, pol );
    setScrollBarPolicy( false, pol );
}


#define mGetScene(act)\
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&scene())\
    if ( !sc ) act;
void uiObjectItemView::resetViewArea( CallBacker* )
{
    int w = 0, h = 0;
    for ( int idx=0; idx<objectitems_.size(); idx++)
    {
	w += objectitems_[idx]->objectSize().width();
	h = mMAX(h,objectitems_[idx]->objectSize().height());
    }
    mGetScene(return); 
    setViewArea( 0, 0, w + sc->layoutPos().x, h + sc->layoutPos().y );
    viewareareset.trigger();
}


uiPoint uiObjectItemView::sceneLayoutPos() const
{
    uiObjectItemView* myself = const_cast<uiObjectItemView*>(this);
    mDynamicCastGet(const uiGraphicsObjectScene*,osc,&myself->scene())
    return osc ? osc->layoutPos() : uiPoint(0,0); 
}


void uiObjectItemView::setSceneLayoutPos( uiPoint pt )
{
    mGetScene(return); sc->setLayoutPos( pt );
}


void uiObjectItemView::addItem( uiObjectItem* itm, int str )
{
    objectitems_ += itm;
    mGetScene(return) sc->addObjectItem(itm);
    sc->setItemStretch( itm, str );
}


void uiObjectItemView::insertItem( uiObjectItem* itm, int posidx, int stretch )
{
    objectitems_.insertAt( itm, posidx );
    mGetScene(return) 
    sc->insertObjectItem( posidx, itm );
    sc->setItemStretch( itm, stretch );
}


void uiObjectItemView::removeItem( uiObjectItem* itm )
{
    objectitems_ -= itm;
    mGetScene(return) sc->removeObjectItem( itm );
}


void uiObjectItemView::removeAllItems()
{
    for ( int idx=nrItems()-1; idx>=0; idx-- )
	removeItem( objectitems_[idx] );
}


uiObjectItem* uiObjectItemView::getItem( int idx ) 
{
    return ( idx>=0 && nrItems()>idx ) ? objectitems_[idx] : 0; 
}


uiObjectItem* uiObjectItemView::getItemFromPos( const Geom::Point2D<int>& pos ) 
{
    mGetScene(return 0)
    Interval<int> borders(0,sc->layoutPos().x); 
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	borders.stop += objectitems_[idx]->objectSize().width(); 
	if ( borders.includes( pos.x,true ) ) return objectitems_[idx];
	borders.start = borders.stop;
    }
    return 0;
}


void uiObjectItemView::getItemsFromRect( const uiRect& rect, 
				       ObjectSet<uiObjectItem>& objs ) 
{
    Interval<float> rectborders( rect.left(), rect.right() );
    Interval<int> objborders(0,0); 
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	objborders.stop += objectitems_[idx]->objectSize().width();
	if ( rectborders.includes( objborders.start,true ) ||
	     rectborders.includes( objborders.stop,true ) )	
	    objs += objectitems_[idx];
	objborders.start = objborders.stop;
    }
}


int uiObjectItemView::stretchFactor( uiObjectItem* itm )
{
    mGetScene(return 0) 
    return sc->stretchFactor( itm );
}


void uiObjectItemView::setStretchFactor( uiObjectItem* itm, int stretchfactor )
{
    mGetScene(return) sc->setItemStretch( itm, stretchfactor );
}


void uiObjectItemView::reSizeItem( int idx, const uiSize& sz )
{
    reSizeItem( getItem( idx ), sz );
}


void uiObjectItemView::reSizeItem( uiObjectItem* itm, const uiSize& sz )
{
    itm->setObjectSize( sz.width(), sz.height() );
}


void uiObjectItemView::setCursor( const MouseCursor& mc )
{
    uiObject::setCursor( mc );
    for ( int idx=0; idx<objectitems_.size(); idx++ )
	objectitems_[idx]->getObject()->setCursor( mc );
}
