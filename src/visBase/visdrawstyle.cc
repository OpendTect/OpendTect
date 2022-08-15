/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "visdrawstyle.h"
#include "iopar.h"

#include <osg/StateSet>
#include <osg/LineStipple>
#include <osg/Point>
#include <osg/LineWidth>

namespace visBase
{

DrawStyle::DrawStyle()
    : pointsizeattrib_(0)
    , pointsize_( 0.0f )
    , linestippleattrib_(0)
    , linewidthattrib_(0)
    , pixeldensity_( DataObject::getDefaultPixelDensity() )
{}


void DrawStyle::setDrawStyle( Style s )
{
    pErrMsg( "Set drawstyle");
}


DrawStyle::Style DrawStyle::getDrawStyle() const
{
    pErrMsg("Implement");
    return Filled;
}


void DrawStyle::setPointSize( float nsz )
{
    if ( !pointsizeattrib_ )
	pointsizeattrib_ = addAttribute( new osg::Point );

    pointsize_ = nsz;

    pointsizeattrib_->setSize(
		nsz * pixeldensity_ / DataObject::getDefaultPixelDensity());
}


float DrawStyle::getPointSize() const
{
    return pointsize_;
}


void DrawStyle::setLineStyle( const OD::LineStyle& nls )
{
    if ( linestyle_ == nls )
	return;

    linestyle_ = nls;
    updateLineStyle();
}


void DrawStyle::setLineWidth( int width )
{
    if ( linestyle_.width_ == width )
	return;

    linestyle_.width_ = width;
    updateLineStyle();
}    



void DrawStyle::updateLineStyle()
{
    if ( linestyle_.width_ == 0 )
    {
	if ( linestippleattrib_ ) removeAttribute( linestippleattrib_ );
	if ( linewidthattrib_ ) removeAttribute( linewidthattrib_ );
	linestippleattrib_ = 0;
	linewidthattrib_ = 0;
	return;
    }

    if ( !linestippleattrib_ )
	linestippleattrib_ = addAttribute( new osg::LineStipple );

    if ( !linewidthattrib_ )
	linewidthattrib_ = addAttribute( new osg::LineWidth );

    const float widthfactor =
	pixeldensity_ / DataObject::getDefaultPixelDensity();

    linewidthattrib_->setWidth( linestyle_.width_ * widthfactor );

    unsigned short pattern;

    if ( linestyle_.type_==OD::LineStyle::None )      pattern = 0;
    else if ( linestyle_.type_==OD::LineStyle::Solid )pattern = 0xFFFF;
    else if ( linestyle_.type_==OD::LineStyle::Dash ) pattern = 0xF0F0;
    else if ( linestyle_.type_==OD::LineStyle::Dot )  pattern = 0xAAAA;
    else if ( linestyle_.type_==OD::LineStyle::DashDot ) pattern = 0xF6F6;
    else pattern = 0xEAEA;

    linestippleattrib_->setPattern( pattern );
}


void DrawStyle::setPixelDensity( float dpi )
{
    if ( dpi == pixeldensity_ )
	return;

    pixeldensity_ = dpi;
    updateLineStyle();

    if ( pointsizeattrib_ )
	setPointSize( getPointSize() );
}

} // namespace visBase
