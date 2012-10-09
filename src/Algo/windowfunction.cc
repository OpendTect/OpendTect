/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id$";

#include "windowfunction.h"

#include "iopar.h"
#include "keystrs.h"


mImplFactory( WindowFunction, WinFuncs );


void WindowFunction::fillPar( IOPar& par ) const
{
    par.set( sKey::Name, name() );
    if ( !hasVariable() )
	return;

    par.set( sKeyVariable(), getVariable() );
}


bool WindowFunction::usePar( const IOPar& par )
{
    if ( !hasVariable() )
	return true;

    float var;
    return par.get( sKeyVariable(), var ) && setVariable( var );
}


#define mImplClass( clss, nm ) \
WindowFunction* clss::create() \
{ \
    return new clss; \
} \
\
\
const char* clss::name() const\
{ return clss::sName(); } \
 \
 \
void clss::initClass() \
{ \
    WinFuncs().addCreator( clss::create, clss::sName() ); \
} \
\
\
const char* clss::sName() { return nm; } \
\
\
float clss::getValue( float x ) const

mImplClass( BoxWindow, "Box" )
{ return fabs(x) > 1 ? 0 : 1; }


mImplClass( HammingWindow, "Hamming" )
{
    float rx = fabs( x );
    if ( rx > 1 )
	return 0;

    return 0.54 + 0.46 * cos( M_PI * rx );
}


mImplClass( HanningWindow, "Hanning" )
{
    float rx = fabs( x );
    if ( rx > 1 ) return 0;

    return (1 + cos( M_PI * rx )) / 2.0;
}


mImplClass( BlackmanWindow, "Blackman" )
{
    float rx = fabs( x );
    if ( rx > 1 ) return 0;

    return 0.42 + 0.5*cos( M_PI * rx )+ 0.08*cos( 2 *M_PI*rx);
}


mImplClass( BartlettWindow, "Bartlett" )
{
    float rx = fabs( x );

    if ( rx > 1 ) return 0;
    return 1-rx;
}


mImplClass( FlatTopWindow, "FlatTop" )
{
    const float pi_x = M_PI * x;
    return (1	+ 1.93	* cos(     pi_x )
		+ 1.29	* cos( 2 * pi_x )
		+ 0.388	* cos( 3 * pi_x )
		+ 0.032	* cos( 4 * pi_x )) / 4.64;
}


mImplClass( CosTaperWindow, "CosTaper" )
{
    float rx = fabs( x );

    if ( rx > 1 ) return 0;
    if ( rx < threshold_ ) return 1;

    rx -= threshold_;
    rx *= factor_;

    return (1 + cos( M_PI * rx )) * .5;
}


bool CosTaperWindow::setVariable( float threshold )
{
    if ( threshold<0 || threshold>1 ) return false;

    threshold_ = threshold;
    factor_ = 1.0/(1-threshold);

    return true;
}
