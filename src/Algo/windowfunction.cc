/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUnusedVar = "$Id: windowfunction.cc,v 1.7 2012-05-02 15:11:20 cvskris Exp $";

#include "windowfunction.h"

#include "iopar.h"
#include "keystrs.h"


mImplFactory( WindowFunction, WINFUNCS );
void WindowFunction::addAllStdClasses()
{
#define mInitStdWFClass(nm) nm##Window::initClass()
    mInitStdWFClass(Box);
    mInitStdWFClass(Bartlett);
    mInitStdWFClass(CosTaper);
    mInitStdWFClass(Hamming);
    mInitStdWFClass(Hanning);
    mInitStdWFClass(Blackman);
    mInitStdWFClass(FlatTop);
}


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


#define mImplClassMinimal(clss) \
void clss##Window::initClass() \
{ WINFUNCS().addCreator( clss##Window::create, clss##Window::sName() ); } \
\
float clss##Window::getValue( float x ) const \
{ \
    if ( x <= -1 || x >= 1 ) return 0;
#define mImplClass(clss) \
    mImplClassMinimal(clss) \
    if ( x < 0 ) x = -x;


mImplClassMinimal(Box)
    return 1;
}
mImplClass(Bartlett)
    return 1 - x;
}
mImplClass(Hanning)
    return .5 * (1 + cos( M_PI * x ));
}
mImplClass(Hamming)
    return 0.54 + 0.46 * cos( M_PI * x );
}
mImplClass(Blackman)
    return 0.42 + 0.5 * cos( M_PI * x )+ 0.08 * cos( 2 * M_PI * x );
}
mImplClass(FlatTop)
    const float pi_x = M_PI * x;
    return (1	+ 1.93	* cos(     pi_x )
		+ 1.29	* cos( 2 * pi_x )
		+ 0.388	* cos( 3 * pi_x )
		+ 0.032	* cos( 4 * pi_x )) / 4.64;
}
mImplClass(CosTaper)
    if ( x < threshold_ ) return 1;
    x -= threshold_; x *= factor_;
    return (1 + cos( M_PI * x )) * .5;
}


bool CosTaperWindow::setVariable( float threshold )
{
    if ( threshold<0 || threshold>1 ) return false;

    threshold_ = threshold;
    factor_ = 1.0 / (1-threshold);

    return true;
}
