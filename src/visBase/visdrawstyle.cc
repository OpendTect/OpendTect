/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdrawstyle.cc,v 1.1 2002-04-25 13:42:47 kristofer Exp $";

#include "visdrawstyle.h"

#include "Inventor/nodes/SoDrawStyle.h"


visBase::DrawStyle::DrawStyle()
    : drawstyle( new SoDrawStyle )
{
    drawstyle->ref();
}


visBase::DrawStyle::~DrawStyle()
{
    drawstyle->unref();
}


void visBase::DrawStyle::setDrawStyle( Style s )
{
    if ( s==Filled ) drawstyle->style = SoDrawStyle::FILLED;
    else if ( s==Lines ) drawstyle->style = SoDrawStyle::LINES;
    else if ( s==Points ) drawstyle->style = SoDrawStyle::POINTS;
    else if ( s==Invisible ) drawstyle->style = SoDrawStyle::INVISIBLE;
}


visBase::DrawStyle::Style visBase::DrawStyle::getDrawStyle() const
{
    SoDrawStyle::Style style = (SoDrawStyle::Style) drawstyle->style.getValue();
    if ( style==SoDrawStyle::FILLED ) return Filled;
    if ( style==SoDrawStyle::LINES ) return Lines;
    if ( style==SoDrawStyle::POINTS ) return Points;
    return Invisible;
}


void visBase::DrawStyle::setPointSize( float nsz )
{
    drawstyle->pointSize.setValue( nsz );
}


float visBase::DrawStyle::getPointSize() const
{
    return drawstyle->pointSize.getValue();
}


void visBase::DrawStyle::setLineStyle( const LineStyle& nls )
{
    linestyle = nls;
    updateLineStyle();
}



SoNode* visBase::DrawStyle::getData()
{
    return drawstyle;
}


void visBase::DrawStyle::updateLineStyle()
{
    drawstyle->lineWidth.setValue( linestyle.width );

    unsigned short pattern;

    if ( linestyle.type==LineStyle::None )      pattern = 0;
    else if ( linestyle.type==LineStyle::Solid )pattern = 0xFFFF;
    else if ( linestyle.type==LineStyle::Dash ) pattern = 0xF0F0;
    else if ( linestyle.type==LineStyle::Dot )  pattern = 0xAAAA;
    else if ( linestyle.type==LineStyle::DashDot ) pattern = 0xF6F6;
    else pattern = 0xEAEA;

    drawstyle->linePattern.setValue( pattern );
}


