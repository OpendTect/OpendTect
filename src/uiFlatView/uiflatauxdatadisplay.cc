/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatauxdatadisplay.cc,v 1.9 2012-08-10 03:50:04 cvsaneesh Exp $";

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

    //TODO Legacy, clean up this
    TypeSet<MarkerStyle2D> markerstyles = markerstyles_;
    const int nrmarkerstyles = markerstyles.size();
    if ( nrmarkerstyles == 0 && poly_.size() == 1 )
	markerstyles += MarkerStyle2D(MarkerStyle2D::Square,4,Color::Black());

    for ( int idx=0; idx<poly_.size() && idx<markerstyles.size(); idx++ )
    {
	const MarkerStyle2D& style = markerstyles[idx];
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
	if ( poly_.size() > listpos )
	    nameitem_->setPos( poly_[listpos] );
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

    display_->setPos( (float) xpos, (float) ypos );
    display_->setScale( (float) xscale, (float) yscale );
}


} //namespace
