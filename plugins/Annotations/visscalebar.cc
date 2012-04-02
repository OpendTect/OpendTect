/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visscalebar.cc,v 1.1 2012-04-02 22:39:38 cvsnanne Exp $";

#include "visscalebar.h"

#include "pickset.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vispolyline.h"
#include "vistransform.h"

mCreateFactoryEntry( Annotations::ScaleBar );
mCreateFactoryEntry( Annotations::ScaleBarDisplay );

namespace Annotations
{

ScaleBar::ScaleBar()
    : visBase::VisualObjectImpl(true)
    , displaytrans_(0)
{
    linestyle_ = visBase::DrawStyle::create();
    linestyle_->ref();
    insertChild( 0, linestyle_->getInventorNode() );

    marker1_ = visBase::Marker::create();
    marker1_->setMaterial( 0 );
    marker1_->ref();
    addChild( marker1_->getInventorNode() );

    marker2_ = visBase::Marker::create();
    marker2_->setMaterial( 0 );
    marker2_->ref();
    addChild( marker2_->getInventorNode() );

    polyline_ = visBase::IndexedPolyLine::create();
    polyline_->setMaterial( 0 );
    polyline_->ref();
    addChild( polyline_->getInventorNode() );
}


ScaleBar::~ScaleBar()
{
    linestyle_->unRef();
    marker1_->unRef();
    marker2_->unRef();
    polyline_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
}


void ScaleBar::setPick( const Pick::Location& loc )
{
    marker1_->setCenterPos( loc.pos );
    marker2_->setCenterPos( loc.pos );

    polyline_->setCoordIndex( 0, 0 );
    polyline_->setCoordIndex( 1, 1 );
    polyline_->getCoordinates()->setPos( 0, loc.pos );
    polyline_->getCoordinates()->setPos( 1, loc.pos );
}


void ScaleBar::setLineWidth( int width )
{ linestyle_->setLineStyle( LineStyle(LineStyle::Solid,width) ); }

void ScaleBar::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
	return;

    marker1_->setDisplayTransformation( nt );
    marker2_->setDisplayTransformation( nt );
    polyline_->setDisplayTransformation( nt );

    displaytrans_ = nt;
    if ( displaytrans_ )
	displaytrans_->ref();
}



// ScaleBarDisplay
ScaleBarDisplay::ScaleBarDisplay()
    : orientation_(Horizontal)
{
    setLineWidth( 2 );
}


ScaleBarDisplay::~ScaleBarDisplay()
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ScaleBarDisplay,zScaleCB) );
}


void ScaleBarDisplay::setOrientation( Orientation ortn )
{
    if ( orientation_ == ortn )
	return;

    orientation_ = ortn;

    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx));
	if ( !sb ) continue;
    }
}


void ScaleBarDisplay::setScene( visSurvey::Scene* ns )
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,ScaleBarDisplay,zScaleCB) );
    visSurvey::SurveyObject::setScene( ns );

    if ( scene_ )
	scene_->zstretchchange.notify( mCB(this,ScaleBarDisplay,zScaleCB) );
}


ScaleBarDisplay::Orientation ScaleBarDisplay::getOrientation() const
{ return orientation_; }


void ScaleBarDisplay::setLineWidth( int width )
{
    linewidth_ = width;
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx));
	if ( sb ) sb->setLineWidth( width );
    }
}


int ScaleBarDisplay::getLineWidth() const
{ return linewidth_; }


void ScaleBarDisplay::zScaleCB( CallBacker* )
{ fullRedraw(); }


void ScaleBarDisplay::dispChg( CallBacker* cb )
{
    fullRedraw();
    LocationDisplay::dispChg( cb );
}


visBase::VisualObject* ScaleBarDisplay::createLocation() const
{
    ScaleBar* sb = ScaleBar::create();
    return sb;
}
	

void ScaleBarDisplay::setPosition( int idx, const Pick::Location& loc )
{
    mDynamicCastGet(ScaleBar*,sb,group_->getObject(idx));
    if ( sb ) sb->setPick( loc );
}

} // namespace Annotation
