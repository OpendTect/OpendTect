/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdrawstyle.cc,v 1.4 2003-11-07 12:22:02 bert Exp $";

#include "visdrawstyle.h"
#include "iopar.h"

#include "Inventor/nodes/SoDrawStyle.h"

DefineEnumNames( visBase::DrawStyle, Style, 1, "Style" )
{ "Filled", "Lines", "Points", "Invisible", 0 };

mCreateFactoryEntry( visBase::DrawStyle );


const char* visBase::DrawStyle::linestylestr = "Line Style";
const char* visBase::DrawStyle::drawstylestr = "Draw Style";
const char* visBase::DrawStyle::pointsizestr = "Point Size";

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

int visBase::DrawStyle::usePar( const IOPar& par )
{
    const char* linestylepar = par.find( linestylestr );
    if ( !linestylepar ) return -1;

    linestyle.fromString( linestylepar );
    updateLineStyle();

    const char* stylepar = par.find( drawstylestr );
    if ( !stylepar ) return -1;

    int enumid = getEnumDef( stylepar, StyleNames, 0, 1, -1 );
    if ( enumid<0 ) return -1;

    setDrawStyle( (Style) enumid );

    float pointsize;
    if ( !par.get( pointsizestr, pointsize ) )
	return -1;
    setPointSize( pointsize );

    return 1;
}


void visBase::DrawStyle::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( par, saveids );

    BufferString linestyleval;
    linestyle.toString( linestyleval );
    par.set( linestylestr, linestyleval );

    par.set( drawstylestr, StyleNames[(int) getDrawStyle()] );
    par.set( pointsizestr, getPointSize() );
}
