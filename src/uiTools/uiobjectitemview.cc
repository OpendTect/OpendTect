/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjectitemview.cc,v 1.1 2010-01-29 14:33:56 cvsbruno Exp $";


#include "uiobjectitemview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "geometry.h"


uiObjectItemView::uiObjectItemView( uiParent* p )
    : uiGraphicsView(p, "Object Item Viewer" )
{
    setScene( *new uiGraphicsObjectScene("Object Scene") );
} 


#define mGetScene()\
    mDynamicCastGet(uiGraphicsObjectScene*,sc,&scene())\
    if ( !sc ) return;
void uiObjectItemView::addItem( uiObjectItem* itm, int stretch )
{
    objectitems_ += itm;
    mGetScene() sc->addObjectItem(itm);
    sc->setItemStretch( itm, stretch );
}


void uiObjectItemView::insertItem( uiObjectItem* itm, int stretch, int posidx)
{
    objectitems_.insertAt( itm, posidx );
    mGetScene() sc->insertObjectItem( posidx, itm );
}


void uiObjectItemView::removeItem( uiObjectItem* itm )
{
    objectitems_ -= itm;
    mGetScene() sc->removeObjectItem( itm );
}


uiObjectItem* uiObjectItemView::getItemFromPos( const Geom::Point2D<int>& pos ) 
{
    //TODO in the y direction
    Interval<int> borders(0,0); 
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	borders.stop += objectitems_[idx]->objectSize().width(); 
	if ( borders.includes( pos.x ) ) return objectitems_[idx];
	borders.start = borders.stop;
    }
    return 0;
}

