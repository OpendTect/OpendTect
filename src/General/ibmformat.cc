/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y word functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ibmformat.h"

#define mcBuf ((const unsigned char*)buf)
#define mBuf ((unsigned char*)buf)

typedef union { int i; float f; } IFUnion;


int IbmFormat::asInt( const void* buf )
{
    if (mcBuf[0] < 128) {
	return((int) mcBuf[3] +
	    256 * ((int) mcBuf[2] +
	    256 * ((int) mcBuf[1] +
	    256 *  (int) mcBuf[0] )));
    } else {
	return((int) mcBuf[3] +
	    256 * ((int) mcBuf[2] +
	    256 * ((int) mcBuf[1] +
	    256 * ((int) (mcBuf[0] & 0x7F) - 128) )));
    }
}


void IbmFormat::putInt( int value, void* buf )
{
    if (value >= 0) {
	mBuf[0] = mCast( unsigned char, value / (256 * 256 * 256) );
	mBuf[1] = value / (256 * 256       )  % 256;
	mBuf[2] = value /  256                % 256;
	mBuf[3] = value                       % 256;
    } else {
	value = (-value);
	value--;

	mBuf[0] = mCast( unsigned char, 255 - value / (256 * 256 * 256) );
	mBuf[1] = 255 - value / (256 * 256       )  % 256;
	mBuf[2] = 255 - value /  256                % 256;
	mBuf[3] = 255 - value                       % 256;
    }
}


short IbmFormat::asShort( const void* buf )
{
    if (mcBuf[0] < 128) {
	return((int) mcBuf[1] +
	    256 * (int) mcBuf[0]);
    } else {
	return((int) mcBuf[1] +
	    256 * ((int) (mcBuf[0] & 0x7F) - 128));
    }
}


void IbmFormat::putShort( short value, void* buf )
{
    if (value >= 0) {
	mBuf[0] = mCast( unsigned char, value / 256 );
	mBuf[1] = value % 256;
    } else {
	value = (-value);
	value--;

	mBuf[0] = mCast( unsigned char, 255 - value / 256 );
	mBuf[1] = 255 - value % 256;
    }
}


unsigned short IbmFormat::asUnsignedShort( const void* buf )
{
    return (int)mcBuf[1] + 256 * (int)mcBuf[0];
}


void IbmFormat::putUnsignedShort( unsigned short value, void* buf )
{
    mBuf[0] = mCast( unsigned char, value / 256 );
    mBuf[1] = value % 256;
}


static void handleEndianness( int& fconv )
{
#ifdef __little__
    int tmp = fconv << 24;
    tmp |= (fconv >> 24) & 0xff;
    tmp |= (fconv & 0xff00) << 8;
    tmp |= (fconv & 0xff0000) >> 8;
    fconv = tmp;
#endif
}


float IbmFormat::asFloat( const void* buf )
{
    int fconv = *( reinterpret_cast<const int*>(buf) );
    if ( fconv )
    {
	handleEndianness( fconv );
	int t = (int) ((0x7f000000 & fconv) >> 22) - 130;
	if ( t <= 0 )
	    fconv = 0;
	else
	{
	    int fmant = 0x00ffffff & fconv;
	    while ( !(fmant & 0x00800000) && t != -1 ) { --t; fmant <<= 1; }
	    if ( t > 254 ) fconv = (0x80000000 & fconv) | 0x7f7fffff;
	    else if (t <= 0) fconv = 0;
	    else fconv = (0x80000000 & fconv) |(t << 23)|(0x007fffff & fmant);
	}
    }

    IFUnion un; un.i = fconv;
    return un.f;
}


void IbmFormat::putFloat( float value, void* inpbuf )
{
    IFUnion un; un.f = value;
    int fconv = un.i;
    if ( fconv )
    {
	int fmant = (0x007fffff & fconv) | 0x00800000;
	int t = ((0x7f800000 & fconv) >> 23) - 126;
	while ( t & 0x3 ) { ++t; fmant >>= 1; }
	fconv = (0x80000000 & fconv) | (((t>>2) + 64) << 24) | fmant;
	handleEndianness( fconv );
    }

    const unsigned char* fbuf = reinterpret_cast<const unsigned char*>( &fconv );
    unsigned char* outbuf = reinterpret_cast<unsigned char*>( inpbuf );
    outbuf[0] = fbuf[0]; outbuf[1] = fbuf[1];
    outbuf[2] = fbuf[2]; outbuf[3] = fbuf[3];
}
