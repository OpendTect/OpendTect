/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visdrawstyle.h"
#include "iopar.h"

#include <osg/StateSet>
#include <osg/LineStipple>
#include <osg/Point>
#include <osg/LineWidth>

namespace visBase
{

DefineEnumNames( DrawStyle, Style, 1, "Style" )
{ "Filled", "Lines", "Points", "Invisible", 0 };

const char* DrawStyle::linestylestr()  { return "Line Style"; }
const char* DrawStyle::drawstylestr()  { return "Draw Style"; }
const char* DrawStyle::pointsizestr()  { return "Point Size"; }

DrawStyle::DrawStyle()
    : pointsize_(0)
    , linestipple_(0)
    , linewidth_(0)
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
    if ( !pointsize_ )
	pointsize_ = addAttribute( new osg::Point );

    pointsize_->setSize( nsz );
}


float DrawStyle::getPointSize() const
{
    return pointsize_ ? pointsize_->getSize() : 1;
}


void DrawStyle::setLineStyle( const LineStyle& nls )
{
    linestyle_ = nls;
    updateLineStyle();
}


void DrawStyle::setLineWidth( int width )
{
    linestyle_.width_ = width;
    updateLineStyle();
}    



void DrawStyle::updateLineStyle()
{

    if ( !linestipple_ )
	linestipple_ = addAttribute( new osg::LineStipple );

    if ( !linewidth_ )
	linewidth_ = addAttribute( new osg::LineWidth );

    linewidth_->setWidth( linestyle_.width_ );

    unsigned short pattern;

    if ( linestyle_.type_==LineStyle::None )      pattern = 0;
    else if ( linestyle_.type_==LineStyle::Solid )pattern = 0xFFFF;
    else if ( linestyle_.type_==LineStyle::Dash ) pattern = 0xF0F0;
    else if ( linestyle_.type_==LineStyle::Dot )  pattern = 0xAAAA;
    else if ( linestyle_.type_==LineStyle::DashDot ) pattern = 0xF6F6;
    else pattern = 0xEAEA;

    linestipple_->setPattern( pattern );

    pErrMsg( "Set Factor as well?" );

 }


int DrawStyle::usePar( const IOPar& par )
{
    const char* linestylepar = par.find( linestylestr() );
    if ( !linestylepar ) return -1;

    linestyle_.fromString( linestylepar );
    updateLineStyle();

    const char* stylepar = par.find( drawstylestr() );
    if ( !stylepar ) return -1;

    int enumid = getIndexInStringArrCI( stylepar, StyleNames(), 0, 1, -1 );
    if ( enumid<0 ) return -1;

    setDrawStyle( (Style)enumid );

    float pointsize;
    if ( !par.get( pointsizestr(), pointsize ) )
	return -1;
    setPointSize( pointsize );

    return 1;
}


void DrawStyle::fillPar( IOPar& par ) const
{
    BufferString linestyleval;
    linestyle_.toString( linestyleval );
    par.set( linestylestr(), linestyleval );

    par.set( drawstylestr(), StyleNames()[(int)getDrawStyle()] );
    par.set( pointsizestr(), getPointSize() );
}

}; // namespace visBase
