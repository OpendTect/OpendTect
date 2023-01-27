/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatauxdatadisplay.h"

#include "uigraphicsitemimpl.h"
#include "uimain.h"
#include "uiflatviewer.h"

namespace FlatView
{

AuxData* uiAuxDataDisplay::clone() const
{ return new uiAuxDataDisplay( *this ); }


uiAuxDataDisplay::uiAuxDataDisplay( const char* nm )
    : AuxData(nm)
{}


uiAuxDataDisplay::uiAuxDataDisplay( const uiAuxDataDisplay& a )
    : AuxData(a)
{}


uiAuxDataDisplay::~uiAuxDataDisplay()
{
    detachAllNotifiers();
}


uiGraphicsItemGroup* uiAuxDataDisplay::getDisplay()
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return nullptr;
    }

    if ( !display_ )
	display_ = new uiGraphicsItemGroup( true );

    return display_;
}


void uiAuxDataDisplay::removeDisplay()
{
    display_ = nullptr;
    removeItems();
}


void uiAuxDataDisplay::removeItems()
{
    polygonitem_ = nullptr;
    polylineitem_ = nullptr;
    nameitem_ = nullptr;
    markeritems_.erase();
}


void uiAuxDataDisplay::updateCB( CallBacker* )
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return;
    }

    if ( !getDisplay() ) return;

    if ( !enabled_ || poly_.isEmpty() )
    {
	display_->removeAll( true );
	removeItems();
	return;
    }

    display_->setVisible( turnon_ );
    if ( !display_->isVisible() )
	return;

    display_->setZValue( zvalue_ );

    if ( x1rg_ || x2rg_ || (fitnameinview_ && nameitem_ && namepos_ != NoDraw) )
    {
	if ( viewer_ )
	    mAttachCBIfNotAttached( viewer_->viewChanged,
				    uiAuxDataDisplay::updateTransformCB );

	updateTransformCB(nullptr);
    }

    const bool drawfill = close_ && fillcolor_.isVisible();
    if ( (linestyle_.isVisible() || drawfill) && poly_.size()>1 )
    {
	uiGraphicsItem* item = nullptr;

	if ( close_ )
	{
	    if ( !polygonitem_ )
	    {
		polygonitem_ = new uiPolygonItem( poly_,fillcolor_.isVisible());
		display_->add( polygonitem_ );
	    }
	    else
	    {
		polygonitem_->setPolygon( poly_ );
	    }

	    polygonitem_->setFillColor( fillcolor_, true );
	    polygonitem_->setFillPattern( fillpattern_ );
	    if ( fillgradient_.hasGradient() )
		polygonitem_->setGradientFill( fillgradient_.from_.x,
			fillgradient_.from_.y, fillgradient_.to_.x,
			fillgradient_.to_.y, fillgradient_.stops_,
			fillgradient_.colors_ );

	    item = polygonitem_;

	    if ( polylineitem_ )
	    {
		display_->remove( polylineitem_, true );
		polylineitem_ = nullptr;
	    }
	}
	else
	{
	    if ( !polylineitem_ )
	    {
		polylineitem_ = new uiPolyLineItem( poly_ );
		display_->add( polylineitem_ );
	    }
	    else
	    {
		if ( needsupdatelines_ )
		    polylineitem_->setPolyLine( poly_ );
	    }

	    item = polylineitem_;

	    if ( polygonitem_ )
	    {
		display_->remove( polygonitem_, true );
		polygonitem_ = nullptr;
	    }
	}

	item->setPenStyle( linestyle_, true );
	item->setCursor( cursor_ );
    }
    else
    {
	if ( polygonitem_ )
	{
	    display_->remove( polygonitem_, true );
	    polygonitem_ = nullptr;
	}

	if ( polylineitem_ )
	{
	    display_->remove( polylineitem_, true );
	    polylineitem_ = nullptr;
	}
    }

    const int nrmarkerstyles = markerstyles_.size();
    while ( markeritems_.size() > poly_.size() )
	display_->remove( markeritems_.pop(), true );

    if ( poly_.size()==1 && nrmarkerstyles==0 )
	markerstyles_ += MarkerStyle2D(MarkerStyle2D::Circle,
				       linestyle_.width_+2,linestyle_.color_);

    for ( int idx=0; idx<poly_.size(); idx++ )
    {
	const int styleidx = mMIN(idx,nrmarkerstyles-1);
	if ( styleidx < 0 )
	{
	    if ( idx < markeritems_.size() )
		markeritems_[idx]->setVisible( false );
	    continue;
	}

	const MarkerStyle2D& style = markerstyles_[styleidx];
	if ( !style.isVisible() )
	    continue;

	uiMarkerItem* item;
	if ( idx>=markeritems_.size() )
	{
	    item = new uiMarkerItem( style );
	    markeritems_ += item;
	    display_->add( item );
	}
	else
	    item = markeritems_[idx];

	item->setMarkerStyle( style );
	item->setRotation( style.rotation_ );
	item->setPenColor( style.color_, true );
	item->setFillColor( style.color_ );
	item->setPos( poly_[idx] );
	item->setVisible( true );
	item->setCursor( cursor_ );
    }

    if ( !name_.isEmpty() && namepos_ != NoDraw && !poly_.isEmpty() )
    {
	if ( !nameitem_ )
	{
	    nameitem_ = new uiTextItem;
	    nameitem_->setItemIgnoresTransformations( true );
	    display_->add( nameitem_ );
	}

	nameitem_->setText( mToUiStringTodo(name_) );
	nameitem_->setAlignment( namealignment_ );

	nameitem_->setTextColor( linestyle_.color_ );
	int listpos = 0;
	if ( namepos_ == First || poly_.size() == 1 )
	    listpos = 0;
	else if ( namepos_ == Last )
	    listpos = poly_.size()-1;
	else if ( namepos_ == Center )
	    listpos = poly_.size()/2;

	nameitem_->setPos( poly_[listpos] );
    }
    else if ( nameitem_ )
    {
	display_->remove( nameitem_, true );
	nameitem_ = nullptr;
    }
}


void uiAuxDataDisplay::updateTransformCB( CallBacker* )
{
    //The aux-data is sitting in the viewer's world space.
    //If we have own axes, we need to set the transform
    //so the local space transforms to the worlds.

    double xpos = 0, ypos = 0, xscale = 1, yscale = 1;
    const uiWorldRect& curview = viewer_->curView();

    if ( fitnameinview_ && nameitem_ && namepos_ != NoDraw && !poly_.isEmpty() )
    {
	int listpos = 0;
	if ( namepos_ == First || poly_.size() == 1 )
	    listpos = 0;
	else if ( namepos_ == Last )
	    listpos = poly_.size()-1;
	else if ( namepos_ == Center )
	    listpos = poly_.size()-2;

	FlatView::Point modnamepos = sCast(FlatView::Point,poly_[listpos]);

	uiGraphicsItem* itmatlistpos = display_->getUiItem( listpos );
	if ( itmatlistpos )
	{
	    Interval<float> vwrxrg;
	    vwrxrg.start = sCast(float,curview.topLeft().x);
	    vwrxrg.stop = sCast(float,curview.bottomRight().x);
	    Interval<float> vwryrg;
	    vwryrg.start = sCast(float,curview.topLeft().y);
	    vwryrg.stop = sCast(float,curview.bottomRight().y);

	    Geom::Point2D<float> pt = itmatlistpos->getPos();
	    const bool isitminxview = vwrxrg.includes(pt.x,true);
	    const bool isitminyview = vwryrg.includes(pt.y,true);
	    if ( isitminxview || isitminyview  )
		modnamepos = curview.moveInside( modnamepos );
	}

	nameitem_->setPos( modnamepos );
    }

    if ( x1rg_ || x2rg_ )
    {
	if ( x1rg_ )
	{
	    xscale = curview.width()/x1rg_->width();
	    xpos = curview.left()-xscale*x1rg_->start;
	}
	if( x2rg_ )
	{
	    yscale = curview.height()/x2rg_->width();
	    ypos = curview.top()-yscale*x2rg_->start;
	}
	display_->setPos( (float) xpos, (float) ypos );
	display_->setScale( (float) xscale, (float) yscale );
    }
}

} // namespace FlatView
