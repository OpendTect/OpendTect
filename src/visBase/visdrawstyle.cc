/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visdrawstyle.cc,v 1.18 2011/04/28 07:00:12 cvsbert Exp $";

#include "visdrawstyle.h"
#include "iopar.h"

#include <Inventor/nodes/SoDrawStyle.h>
#include "SoOD.h"

mCreateFactoryEntry( visBase::DrawStyle );

namespace visBase
{

DefineEnumNames( DrawStyle, Style, 1, "Style" )
{ "Filled", "Lines", "Points", "Invisible", 0 };

const char* DrawStyle::linestylestr()  { return "Line Style"; }
const char* DrawStyle::drawstylestr()  { return "Draw Style"; }
const char* DrawStyle::pointsizestr()  { return "Point Size"; }

DrawStyle::DrawStyle()
    : drawstyle( new SoDrawStyle )
{
    drawstyle->ref();
}


DrawStyle::~DrawStyle()
{
    drawstyle->unref();
}


void DrawStyle::setDrawStyle( Style s )
{
    if ( s==Filled ) drawstyle->style = SoDrawStyle::FILLED;
    else if ( s==Lines ) drawstyle->style = SoDrawStyle::LINES;
    else if ( s==Points ) drawstyle->style = SoDrawStyle::POINTS;
    else if ( s==Invisible ) drawstyle->style = SoDrawStyle::INVISIBLE;
}


DrawStyle::Style DrawStyle::getDrawStyle() const
{
    SoDrawStyle::Style style = (SoDrawStyle::Style) drawstyle->style.getValue();
    if ( style==SoDrawStyle::FILLED ) return Filled;
    if ( style==SoDrawStyle::LINES ) return Lines;
    if ( style==SoDrawStyle::POINTS ) return Points;
    return Invisible;
}


void DrawStyle::setPointSize( float nsz )
{
    drawstyle->pointSize.setValue( nsz );
}


float DrawStyle::getPointSize() const
{
    return drawstyle->pointSize.getValue();
}


void DrawStyle::setLineStyle( const LineStyle& nls )
{
    linestyle = nls;
    updateLineStyle();
}

void DrawStyle::setLineWidth( int width )
{
    drawstyle->lineWidth.setValue( width );
}    


SoNode* DrawStyle::gtInvntrNode()
{
    return drawstyle;
}


void DrawStyle::updateLineStyle()
{
    drawstyle->lineWidth.setValue( linestyle.width_ );

    unsigned short pattern;

    if ( linestyle.type_==LineStyle::None )      pattern = 0;
    else if ( linestyle.type_==LineStyle::Solid )pattern = 0xFFFF;
    else if ( linestyle.type_==LineStyle::Dash ) pattern = 0xF0F0;
    else if ( linestyle.type_==LineStyle::Dot )  pattern = 0xAAAA;
    else if ( linestyle.type_==LineStyle::DashDot ) pattern = 0xF6F6;
    else pattern = 0xEAEA;

    drawstyle->linePattern.setValue( pattern );
}


void DrawStyle::getLineWidthBounds( int& min, int& max )
{
    SoOD::getLineWidthBounds( min, max );
}


int DrawStyle::usePar( const IOPar& par )
{
    const char* linestylepar = par.find( linestylestr() );
    if ( !linestylepar ) return -1;

    linestyle.fromString( linestylepar );
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


void DrawStyle::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );

    BufferString linestyleval;
    linestyle.toString( linestyleval );
    par.set( linestylestr(), linestyleval );

    par.set( drawstylestr(), StyleNames()[(int)getDrawStyle()] );
    par.set( pointsizestr(), getPointSize() );
}

}; // namespace visBase
