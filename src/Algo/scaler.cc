/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 8-9-1995
 * FUNCTION : Scaler functions
-*/


#include "scaler.h"
#include "separstr.h"
#include "string2.h"
#include "undefval.h"
#include "keystrs.h"
#include "iopar.h"
#include "math2.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#ifndef LN_MAXDOUBLE
# define LN_MAXDOUBLE 709.7
#endif


Scaler* Scaler::get( const char* str )
{
    if ( !str || ! *str )
	return 0;

    FileMultiString fs( str );
    FixedString typ = fs[0];
    Scaler* scaler = 0;
    if ( typ == sLinScaler )
	scaler = new LinScaler;
    else if ( typ == sLogScaler )
	scaler = new LogScaler;
    else if ( typ == sExpScaler )
	scaler = new ExpScaler;
    else if ( typ == sAsymptScaler )
	scaler = new AsymptScaler;

    if ( scaler )
	scaler->fromString( fs.from(1) );
    return scaler;
}


Scaler* Scaler::get( const IOPar& iop )
{
    return get( iop.find(sKey::Scale()) );
}


void Scaler::put( char* str, int sz ) const
{
    FileMultiString fs( type() );
    fs += FileMultiString( toString() );
#ifdef __win__
    strcpy_s( str, sz, fs );
#else
    strcpy( str, fs );
#endif
}


void Scaler::put( IOPar& iop ) const
{
    BufferString bufstr( 1024, false );
    put( bufstr.getCStr(), bufstr.bufSize() );
    iop.set( sKey::Scale(), bufstr.buf() );
}


void Scaler::putToPar( IOPar& iop, const Scaler* sc )
{
    if ( !sc )
	iop.removeWithKey( sKey::Scale() );
    else
	sc->put( iop );
}


LinScaler::LinScaler( double x0, double y0, double x1, double y1 )
    : factor_(1.)
    , constant_(0.)
{
    set( x0, y0, x1, y1 );
}


LinScaler* LinScaler::inverse() const
{
    if ( factor_ == 0 )
	return new LinScaler( 0, 0 );
    return new LinScaler( 1./factor_, constant_ / factor_ );
}


void LinScaler::set( double x0, double y0, double x1, double y1 )
{
    x1 -= x0;
    if ( x1 )
    {
	factor_ = (y1 - y0) / x1;
	constant_ = y0 - factor_ * x0;
    }
}


double LinScaler::scale( double v ) const
{
    return mIsUdf(v) ? v : constant_ + factor_ * v;
}


double LinScaler::unScale( double v ) const
{
    if ( mIsUdf(v) ) return v;
    v -= constant_; if ( factor_ ) v /= factor_;
    return v;
}


const char* LinScaler::toString() const
{
    static FileMultiString fms;
    fms = "";
    fms += constant_;
    fms += factor_;
    return fms;
}


void LinScaler::fromString( const char* str )
{
    constant_ = 0; factor_ = 1;
    if ( !str || ! *str ) return;
    FileMultiString fms = str;
    const int sz = fms.size();
    if ( sz > 0 ) constant_ = fms.getDValue( 0 );
    if ( sz > 1 ) factor_ = fms.getDValue( 1 );
}


Scaler* LogScaler::inverse() const
{
    return new ExpScaler( ten_ );
}


double LogScaler::scale( double v ) const
{
    if ( v <= 0 || mIsUdf(v) ) return Values::udfVal(v);
    return ten_ ? Math::Log10(v) : Math::Log(v);
}


double LogScaler::unScale( double v ) const
{
    return ExpScaler(ten_).scale(v);
}


const char* LogScaler::toString() const
{
    return getYesNoString( ten_ );
}


void LogScaler::fromString( const char* str )
{
    ten_ = toBool( str, false );
}


Scaler* ExpScaler::inverse() const
{
    return new LogScaler( ten_ );
}


double ExpScaler::scale( double v ) const
{
    if ( mIsUdf(v) ) return Values::udfVal(v);
    return ten_ ? Math::PowerOf(10,v) : Math::Exp(v);
}


double ExpScaler::unScale( double v ) const
{
    return LogScaler(ten_).scale(v);
}


const char* ExpScaler::toString() const
{
    return getYesNoString( ten_ );
}


void ExpScaler::fromString( const char* str )
{
    ten_ = toBool( str, false );
}


void AsymptScaler::set( double c, double w, double l )
{
    width_ = w; center_ = c; linedge_ = l;

    factor_ = mIsZero(width_,1e-30) ? 0 : linedge_ / width_;
}


double AsymptScaler::scale( double v ) const
{
    if ( mIsUdf(v) ) return v;
    // TODO

    v -= center_;
    if ( v <= width_ && v >= -width_ )
	return v * factor_;
    // TODO
    return v;
}


double AsymptScaler::unScale( double v ) const
{
    if ( mIsUdf(v) ) return v;
    // TODO
    // v = unscale_implementation;
    v /= factor_;
    v += center_;
    return v;
}


const char* AsymptScaler::toString() const
{
    static FileMultiString fms;
    fms = "";
    fms += center_;
    fms += width_;
    fms += linedge_;
    return fms;
}


void AsymptScaler::fromString( const char* str )
{
    center_ = 0; width_ = 1; linedge_ = 0.95;
    if ( !str || ! *str ) return;
    FileMultiString fms = str;
    const int sz = fms.size();
    if ( sz > 0 ) center_ = fms.getDValue( 0 );
    if ( sz > 1 ) width_ = fms.getDValue( 1 );
    if ( sz > 2 ) linedge_ = fms.getDValue( 2 );

    set( center_, width_, linedge_ );
}
