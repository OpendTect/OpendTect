/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatauxdatadisplay.cc,v 1.5 2012-07-10 13:55:30 cvsbruno Exp $";

#include "uiflatauxdatadisplay.h"

#include "uigraphicsitemimpl.h"
#include "uimain.h"
#include "uiflatviewer.h"

namespace FlatView
{


AuxData* uiAuxDataDisplay::clone() const
{ return new uiAuxDataDisplay( *this ); }


uiAuxDataDisplay::~uiAuxDataDisplay()
{
    if ( viewer_ ) viewer_->viewChanged.remove(
	    mCB(this,uiAuxDataDisplay,updateTransformCB) );
}


#define mImplConstructor( arg ) \
    : AuxData( arg ) \
    , display_( 0 ) \
    , polygonitem_( 0 ) \
    , polylineitem_( 0 ) \
    , nameitem_( 0 ) \
    , viewer_( 0 )


uiAuxDataDisplay::uiAuxDataDisplay( const char* nm )
    mImplConstructor( nm )
{}


uiAuxDataDisplay::uiAuxDataDisplay( const uiAuxDataDisplay& a )
    mImplConstructor( a )
{}


uiGraphicsItemGroup* uiAuxDataDisplay::getDisplay()
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return 0;
    }

    if ( !display_ )
	display_ = new uiGraphicsItemGroup( true );

    return display_;
}


void uiAuxDataDisplay::updateCB( CallBacker* cb )
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return;
    }

    if ( !display_ )
	display_ = new uiGraphicsItemGroup( true );

    //Todo: remove all ?
    
    if ( !enabled_ || isEmpty() )
    {
	display_->setVisible( false );
	return;
    }

    //dispids_.erase();
    
    display_->setZValue( zvalue_ );
    display_->setVisible( displayed_ );

    //Todo Adapt to scales

    if ( x1rg_ || x2rg_ )
    {
	viewer_->viewChanged.notifyIfNotNotified(
	    mCB(this,uiAuxDataDisplay,updateTransformCB) );

	updateTransformCB(0);
    }
/*
    TypeSet<uiPoint> ptlist;
    const int nrpoints = ad.poly_.size();
    for ( int idx=0; idx<nrpoints; idx++ )
	ptlist += w2u.transform( ad.poly_[idx] ) + datarect.topLeft();
	*/

    const bool drawfill = close_ && fillcolor_.isVisible();
    if ( (linestyle_.isVisible() || drawfill) && poly_.size()>1 )
    {
	uiGraphicsItem* item = 0;

	if ( close_ )
	{
	    //Handle fill ... ?
	    if ( !polygonitem_ )
	    {
		polygonitem_ = new uiPolygonItem( poly_,fillcolor_.isVisible());
		display_->add( polygonitem_ );
		//dispids_ += polygonitem_->id();
	    }
	    else
	    {
		polygonitem_->setPolygon( poly_ );
	    }

	    polygonitem_->setFillColor( fillcolor_, true );
	    item = polygonitem_;

	    if ( polylineitem_ )
	    {
		display_->remove( polylineitem_, true );
		polylineitem_ = 0;
	    }
	}
	else
	{
	    if ( !polylineitem_ )
	    {
		polylineitem_ = new uiPolyLineItem( poly_ );
		display_->add( polylineitem_ );
		//dispids_ += polylineitem_->id();
	    }
	    else
	    {
		polylineitem_->setPolyLine( poly_ );
	    }

	    item = polylineitem_;

	    if ( polygonitem_ )
	    {
		display_->remove( polygonitem_, true );
		polygonitem_ = 0;
	    }
	}

	item->setPenStyle( linestyle_ );
    }
    else
    {
	if ( polygonitem_ )
	{
	    display_->remove( polygonitem_, true );
	    polygonitem_ = 0;
	}

	if ( polylineitem_ )
	{
	    display_->remove( polylineitem_, true );
	    polylineitem_ = 0;
	}
    }
    /*else if ( (ptlist.size()==1) && (ad.markerstyles_.size()==0) )
    {
	const Color usecol = color( true );
	if ( !pointitem_ )
	{
	    pointitem_ = new uiMarkerItem(
		    ptlist[0], MarkerStyle2D(MarkerStyle2D::Square,4,usecol) );
	    canvas_.scene().addItem( pointitem_ );
	    ad.dispids_ += pointitem_->id();
	    pointitem_->setVisible( ad.displayed_ );
	}
	else
	    pointitem_->setPos( ptlist[0] );
	pointitem_->setPenColor( usecol );
	pointitem_->setZValue( 2 );
    } */

    const int nrmarkerstyles = markerstyles_.size();
    const MarkerStyle2D defmarker(MarkerStyle2D::Square,4,Color::Black());
    for ( int idx=0; idx<poly_.size(); idx++ )
    {
	const int styleidx = mMIN(idx,nrmarkerstyles-1);
	const MarkerStyle2D& style = styleidx==-1
	    ? defmarker
	    : markerstyles_[styleidx];

	if ( !style.isVisible() )
	    continue;

	uiMarkerItem* item;
	if ( idx>=markeritems_.size() )
	{
	    item = new uiMarkerItem( style );
	    markeritems_ += item;
	    //dispids_ += item->id();
	    display_->add( item );
	}
	else
	    item = markeritems_[idx];

	item->setRotation( style.rotation_ );
	item->setPenColor( style.color_ );
	item->setFillColor( style.color_ );
	item->setPos( poly_[idx] );
	//item->setZValue( 2 );
	//item->setVisible( ad.displayed_ );
    }

    if ( !name_.isEmpty() && !mIsUdf(namepos_) )
    {
	int listpos = namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > poly_.size() ) listpos = poly_.size()-1;

	if ( !nameitem_ )
	{
	    nameitem_ = new uiTextItem( name_, namealignment_ );
	    display_->add( nameitem_ );
	    //dispids_ += nameitem_->id();
	}
	else
	{
	    nameitem_->setText( name_ );
	    nameitem_->setAlignment( namealignment_ );
	}

	nameitem_->setTextColor( linestyle_.color_ );
    }
    else if ( nameitem_ )
    {
	display_->remove( nameitem_, true );
	nameitem_ = 0;
    }
}

void uiAuxDataDisplay::updateTransformCB( CallBacker* cb )
{
    //The aux-data is sitting in the viewer's world space.
    //If we have own axises, we need to set the transform
    //so the local space transforms to the worlds.
   
    double xpos = 0, ypos = 0, xscale = 1, yscale = 1;
    const uiWorldRect& curview = viewer_->curView();

    if ( x1rg_ )
    {
	xscale = curview.width()/x1rg_->width();
	xpos = curview.left()-xscale*x1rg_->start;
    }

    if ( x2rg_ )
    {
	yscale = curview.height()/x2rg_->width();
	ypos = curview.top()-yscale*x2rg_->start;
    }

    display_->setPos( xpos, ypos );
    display_->setScale( xscale, yscale );
}


} //namespace
