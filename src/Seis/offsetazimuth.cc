/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2007
-*/

static const char* rcsID mUnusedVar = "$Id: offsetazimuth.cc,v 1.7 2012-07-10 08:05:31 cvskris Exp $";

#include "offsetazimuth.h"
#include <math.h>


#define mAzimuthFactor	0.012295862	//2PI/511
#define mAzimuthUndef	511		
#define mAzimuthMask	511		
#define mOffsetFactor	0.1		//Decimeters
#define mOffsetMax      4194302
#define mOffsetMin      -4194302	//Decimeters
#define mOffsetUndef    4194303

OffsetAzimuth::OffsetAzimuth( float off, float azi )
    : offsetazi_( 0 )
{
    setOffset( off );
    setAzimuth( azi );
}


bool OffsetAzimuth::operator==( const OffsetAzimuth& b ) const
{ return b.offsetazi_==offsetazi_; }


bool OffsetAzimuth::operator!=( const OffsetAzimuth& b ) const
{ return b.offsetazi_!=offsetazi_; }


int OffsetAzimuth::asInt() const
{ return offsetazi_; }


void OffsetAzimuth::setFrom( int oa )
{ offsetazi_ = oa; }


float OffsetAzimuth::offset() const
{
    int offset_10 = offsetazi_>>9;
    if ( offset_10==mOffsetUndef )
	return mUdf(float);

    return ((float)offset_10) * mOffsetFactor;
}


float OffsetAzimuth::azimuth() const
{
    unsigned int azi = offsetazi_;
    azi &= mAzimuthMask;
    return azi==mAzimuthUndef ? mUdf(float) : azi * mAzimuthFactor;
}


bool OffsetAzimuth::isOffsetUndef() const
{
    const int offset_10 = offsetazi_>>9;
    return offset_10==mOffsetUndef;
}
	

bool OffsetAzimuth::isAzimuthUndef() const
{
    unsigned int azi = offsetazi_;
    azi &= mAzimuthMask;
    return azi==mAzimuthUndef;
}


void OffsetAzimuth::setOffset( float off )
{
    int ioffset;
    if ( mIsUdf(off) )
	ioffset = mOffsetUndef;
    else
    {
	off /= mOffsetFactor;
	ioffset = mNINT32( off );
	if ( ioffset>mOffsetMax )
	    ioffset = mOffsetMax;
	else if ( ioffset<mOffsetMin )
	    ioffset = mOffsetMin;
    }

    ioffset = ioffset << 9;
    unsigned int iazimuth = offsetazi_;
    iazimuth &= mAzimuthMask;

    offsetazi_ = ioffset+iazimuth;
}
    

void OffsetAzimuth::setAzimuth( float azi )
{
    offsetazi_ &= 0xFFFFFE00;
    if ( mIsUdf(azi) )
    {
	offsetazi_ += mAzimuthUndef;
	return;
    }
	
    const float twopi = M_PI * 2;
    while ( azi<0 ) azi += twopi;
    while ( azi>=twopi ) azi -= twopi;
    azi /= mAzimuthFactor;
    int iazimuth = mNINT32( azi );

    offsetazi_ += iazimuth;
}


float OffsetAzimuth::distanceTo( const OffsetAzimuth& b, bool sq ) const
{
    const Coord myvec = srcRcvPos( Coord(0,0), true );
    if ( !myvec.isDefined() )
	return mUdf(float);

    const Coord bvec = b.srcRcvPos( Coord(0,0), true );
    if ( !bvec.isDefined() )
	return mUdf(float);

    return sq ? myvec.sqDistTo( bvec ) : myvec.distTo( bvec );
}


Coord OffsetAzimuth::srcRcvPos( const Coord& center, bool add ) const
{
    if ( isOffsetUndef() || isAzimuthUndef() )
	return Coord::udf();

    const float azi = azimuth();
    const float off = offset();

    const Coord vector( cos(azi)*off, sin(azi)*off );
    if ( add ) return center+vector;
    return center-vector;
}
