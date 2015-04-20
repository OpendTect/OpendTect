/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    mInitStdWFClass(Kaiser);
}


void WindowFunction::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name() );
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
    return (float) ( .5 * (1 + cos( M_PI * x )) );
}
mImplClass(Hamming)
    return (float) ( 0.54 + 0.46 * cos( M_PI * x ) );
}
mImplClass(Blackman)
    return (float) ( 0.42 + 0.5 * cos( M_PI * x ) +
				  0.08 * cos( 2 * M_PI * x ) );
}
mImplClass(FlatTop)
    const float pi_x = (float) ( M_PI * x );
    return (1	+ 1.93f	 * cos(     pi_x )
		+ 1.29f	 * cos( 2 * pi_x )
		+ 0.388f * cos( 3 * pi_x )
		+ 0.032f * cos( 4 * pi_x )) / 4.64f;
}
mImplClass(CosTaper)
    if ( x < threshold_ )
       return 1;
    x -= threshold_; x *= factor_;
    return (float) ( (1 + cos( M_PI * x )) * .5 );
}
mImplClass(Kaiser)
    if ( x > 0.5 )
       return 0.f;

    double xx = mCast(double,nrsamp_-1) * mCast(double,x);
    xx *= xx;
    const double val = scale_*Math::BesselI0( alpha_*Math::Sqrt(1-xx/xxmax_) );

    return mCast(float,val);
}


bool CosTaperWindow::setVariable( float threshold )
{
    if ( threshold<0 || threshold>1 )
	return false;

    threshold_ = threshold;
    factor_ = 1.0f / (1-threshold);

    return true;
}


KaiserWindow::KaiserWindow()
    : width_(mUdf(float))
{
    setNumberSamples( 11 );
    setVariable( 0.4 );
}


bool KaiserWindow::setVariable( float width )
{
    if ( width<0. || width>1.f )
	return false;

    width_ = width;
    double a = 14.36 * mCast(double,width_) * mCast(double,nrsamp_) + 7.95;
    if ( a <= 21. )
	alpha_ = 0.;
    else if ( a <= 50. )
    {
	a -= 21.;
	alpha_ = 0.5842 * Math::PowerOf(a,0.4) + 0.07886 * a;
    }
    else
    {
	a -= 8.7;
	alpha_ = 0.1102 * a;
    }

    scale_ = 1./ Math::BesselI0(alpha_);

    return true;
}


void KaiserWindow::setNumberSamples( int ns )
{
    nrsamp_ = ns;
    const double nrsampd = mCast(double,ns);
    xxmax_ = 0.25 * nrsampd * nrsampd;
    if ( !mIsUdf(width_) )
	setVariable( width_ );
}


float KaiserWindow::getError() const
{
    if ( mIsUdf(width_) )
	return mUdf(float);

    const double a = 14.36 * mCast(double,width_) * mCast(double,nrsamp_) +7.95;
    return (float)Math::PowerOf( 10, -a/20 );
}

