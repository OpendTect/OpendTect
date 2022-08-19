/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "windowfunction.h"

#include "iopar.h"
#include "keystrs.h"


mImplFactory( WindowFunction, WINFUNCS );

bool WindowFunction::hasVariable( const BufferString& nm )
{
    WindowFunction* winfunc = WINFUNCS().create( nm );
    const bool hastapervar = winfunc && winfunc->hasVariable();
    delete winfunc;

    return hastapervar;
}

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
{ \
    WINFUNCS().addCreator( clss##Window::create, clss##Window::sName() ); \
} \
\
float clss##Window::getValue( float x ) const \
{ \
    if ( x < -1 || x > 1 ) return 0;
#define mImplClass(clss) \
    mImplClassMinimal(clss) \
    if ( x < 0 ) x = -x; \
    double val = mCast(double,x);

void BoxWindow::initClass()
{
    WINFUNCS().addCreator( BoxWindow::create, BoxWindow::sName() );
}


float BoxWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    return 1;
}


void BartlettWindow::initClass()
{
    WINFUNCS().addCreator( BartlettWindow::create, BartlettWindow::sName() );
}


float BartlettWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    return mCast(float, 1. - val );
}


void HanningWindow::initClass()
{
    WINFUNCS().addCreator( HanningWindow::create, HanningWindow::sName() );
}


float HanningWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    return mCast(float, 0.50 * (1. + cos( M_PI * val )) );
}


void HammingWindow::initClass()
{
    WINFUNCS().addCreator( HammingWindow::create, HammingWindow::sName() );
}


float HammingWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    return mCast(float, 0.54 + 0.46 * cos( M_PI * val ) );
}


void BlackmanWindow::initClass()
{
    WINFUNCS().addCreator( BlackmanWindow::create, BlackmanWindow::sName() );
}


float BlackmanWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    return mCast(float, 0.42 + 0.50 * cos( M_PI  * val ) +
			       0.08 * cos( M_2PI * val ) );
}


void FlatTopWindow::initClass()
{
    WINFUNCS().addCreator( FlatTopWindow::create, FlatTopWindow::sName() );
}


float FlatTopWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    const double pi_x = M_PI * val;
    return mCast(float, ( 1.	+ 1.93	* cos(	    pi_x )
				+ 1.29	* cos( 2. * pi_x )
				+ 0.388 * cos( 3. * pi_x )
				+ 0.032 * cos( 4. * pi_x )) / 4.64 );
}


void CosTaperWindow::initClass()
{
    WINFUNCS().addCreator( CosTaperWindow::create, CosTaperWindow::sName() );
}


float CosTaperWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    if ( val < threshold_ )
       return 1;
    val -= threshold_; val *= factor_;
    return mCast(float, (1. + cos( M_PI * val )) * 0.5 );
}


void KaiserWindow::initClass()
{
    WINFUNCS().addCreator( KaiserWindow::create, KaiserWindow::sName() );
}


float KaiserWindow::getValue( float x ) const
{
    if ( x < -1 || x > 1 ) return 0;
    if ( x < 0 ) x = -x;
    double val = mCast(double,x);
    const double num = Math::BesselI0( M_PI * alpha_ * Math::Sqrt(1-val*val) );
    return mCast(float, num / denom_ );
}



CosTaperWindow::CosTaperWindow()
{
    setVariable( 0.05f );
}


bool CosTaperWindow::setVariable( float threshold )
{
    if ( threshold<0 || threshold>1 )
	return false;

    threshold_ = threshold;
    factor_ = 1.0f / (1-threshold);

    return true;
}


bool CosTaperWindow::isAcceptableVariable( float val ) const
{
    return val >= 0.f && val <= 1.f;
}


bool CosTaperWindow::isLegacyTaper(const BufferString& nm )
{
    return nm == BufferString( "CosTaper5" ) ||
	   nm == BufferString( "CosTaper10" ) ||
	   nm == BufferString( "CosTaper20" );
}


float CosTaperWindow::getLegacyTaperVariable( const BufferString& nm )
{
    if ( !isLegacyTaper(nm) )
	return mUdf(float);

    if ( nm == BufferString("CosTaper5") )
	return 0.95;
    else if ( nm == BufferString("CosTaper10") )
	return 0.9f;
    else if ( nm == BufferString("CosTaper20") )
	return 0.8f;

    return mUdf(float);
}



KaiserWindow::KaiserWindow()
    : width_(mUdf(double))
    , ns_(mUdf(int))
{
    setVariable( 1.5f );
}



KaiserWindow::KaiserWindow( double twidth, int nrsamples )
    : width_(mUdf(double))
    , ns_(mUdf(int))
{
    set( twidth, nrsamples );
}


bool KaiserWindow::setVariable( float alpha )
{
    if ( !isAcceptableVariable(alpha) )
	return false;

    alpha_ = mCast(double,alpha);
    denom_ = Math::BesselI0( M_PI * alpha_ );

    return true;
}


bool KaiserWindow::set( double width, int nrsamples )
{
    double a = 14.36 * width * mCast(double,nrsamples) + 7.95;
    double alpha;
    if ( a <= 21. )
	alpha = 0.;
    else if ( a <= 50. )
    {
	a -= 21.;
	alpha = 0.5842 * Math::PowerOf(a,0.4) + 0.07886 * a;
    }
    else
    {
	a -= 8.7;
	alpha = 0.1102 * a;
    }

    if ( !setVariable(mCast(float,alpha/M_PI)) )
	return false;

    width_ = width;
    ns_ = nrsamples;

    return true;
}


bool KaiserWindow::isAcceptableVariable( float val ) const
{
    return val >= 0.f;
}


double KaiserWindow::getWidth( int nrsamples ) const
{
    if ( mIsUdf(nrsamples) )
	return mUdf(double);

    double res = M_PI * alpha_ / 0.1102 + 8.7;
    return ( res - 7.95 ) / ( 14.36 * mCast(double,nrsamples) );
}


double KaiserWindow::getError( int nrsamples ) const
{
    const double width = getWidth( nrsamples );
    if ( mIsUdf(width) )
	return mUdf(double);

    const double a = 14.36 * width * mCast(double,nrsamples) + 7.95;
    return Math::PowerOf( 10., -a/20. );
}


double KaiserWindow::getError() const
{ return getError( ns_ ); }
