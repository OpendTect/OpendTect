/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 8-9-1995
 * FUNCTION : Scaler functions
-*/
 
static const char* rcsID = "$Id: scaler.cc,v 1.2 2004-06-16 14:54:18 bert Exp $";

#include "scaler.h"
#include "separstr.h"
#include "string2.h"
#include "genc.h"
#include <stdio.h>
#include <math.h>
#include <limits.h>

#ifndef LN_MAXDOUBLE
# define LN_MAXDOUBLE 709.7
#endif


Scaler* Scaler::get( const char* str )
{
    if ( !str || ! *str ) return new LinScaler;

    FileMultiString fs( str );
    const char* typ = fs[0];
    Scaler* scaler = 0;
    if ( !strcmp(typ,sLinScaler) )
	scaler = new LinScaler;
    else if ( !strcmp(typ,sLogScaler) )
	scaler = new LogScaler;
    else if ( !strcmp(typ,sExpScaler) )
	scaler = new ExpScaler;
    else if ( !strcmp(typ,sAsymptScaler) )
	scaler = new AsymptScaler;

    if ( scaler ) scaler->fromString( fs.from(1) );
    return scaler;
}


void Scaler::put( char* str ) const
{
    FileMultiString fs( type() );
    fs += toString();
    strcpy( str, fs );
}


LinScaler::LinScaler( double x0, double y0, double x1, double y1 )
{
    set( x0, y0, x1, y1 );
}


void LinScaler::set( double x0, double y0, double x1, double y1 )
{
    factor = (y1-y0)/(x1-x0);
    constant = y0-factor*x0;
}


double LinScaler::scale( double v ) const
{
    return mIsUndefined(v) ? v : constant + factor * v;
}


double LinScaler::unScale( double v ) const
{
    if ( mIsUndefined(v) ) return v;
    v -= constant; if ( factor ) v /= factor;
    return v;
}


const char* LinScaler::toString() const
{
    static FileMultiString fms;
    fms = "";
    fms += constant;
    fms += factor;
    return fms;
}


void LinScaler::fromString( const char* str )
{
    constant = 0; factor = 1;
    if ( !str || ! *str ) return;
    FileMultiString fms = str;
    const int sz = fms.size();
    if ( sz > 0 ) constant = atof( fms[0] );
    if ( sz > 1 ) factor = atof( fms[1] );
}


double LogScaler::scale( double v ) const
{
    if ( v <= 0 || mIsUndefined(v) ) return mUndefValue;
    return ten ? log10(v) : log(v);
}


double LogScaler::unScale( double v ) const
{
    return ExpScaler(ten).scale(v);
}


const char* LogScaler::toString() const
{
    return getYesNoString( ten );
}


void LogScaler::fromString( const char* str )
{
    ten = yesNoFromString( str );
}


double ExpScaler::scale( double v ) const
{
    if ( mIsUndefined(v) ) return mUndefValue;
    double maxv = LN_MAXDOUBLE;
    if ( ten ) maxv /= 2.3025851;
    if ( v > maxv ) return mUndefValue;
    return ten ? PowerOf(10,v) : exp(v);
}


double ExpScaler::unScale( double v ) const
{
    return LogScaler(ten).scale(v);
}


const char* ExpScaler::toString() const
{
    return getYesNoString( ten );
}


void ExpScaler::fromString( const char* str )
{
    ten = yesNoFromString( str );
}


void AsymptScaler::set( double c, double w, double l )
{
    width_ = w; center_ = c; linedge_ = l;

    factor = mIsZero(width_,1e-30) ? 0 : linedge_ / width_;
}


double AsymptScaler::scale( double v ) const
{
    if ( mIsUndefined(v) ) return v;
    // TODO

    v -= center_;
    if ( v <= width_ && v >= -width_ )
	return v * factor;
    // TODO
    return v;
}


double AsymptScaler::unScale( double v ) const
{
    if ( mIsUndefined(v) ) return v;
    // TODO
    // v = unscale_implementation;
    v /= factor;
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
    if ( sz > 0 ) center_ = atof( fms[0] );
    if ( sz > 1 ) width_ = atof( fms[1] );
    if ( sz > 2 ) linedge_ = atof( fms[2] );

    set( center_, width_, linedge_ );
}
