/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y word functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ibmformat.h"
#include <string.h>

#define mcBuf ((const unsigned char*)buf)
#define mBuf ((unsigned char*)buf)

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
	mBuf[0] = value / (256 * 256 * 256)        ;
	mBuf[1] = value / (256 * 256       )  % 256;
	mBuf[2] = value /  256                % 256;
	mBuf[3] = value                       % 256;
    } else {
	value = (-value);
	value--;

	mBuf[0] = 255 - value / (256 * 256 * 256)        ;
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
	mBuf[0] = value / 256;
	mBuf[1] = value % 256;
    } else {
	value = (-value);
	value--;

	mBuf[0] = 255 - value / 256;
	mBuf[1] = 255 - value % 256;
    }
}


unsigned short IbmFormat::asUnsignedShort( const void* buf )
{
    return (int)mcBuf[1] + 256 * (int)mcBuf[0];
}


void IbmFormat::putUnsignedShort( unsigned short value, void* buf )
{
    mBuf[0] = value / 256;
    mBuf[1] = value % 256;
}



float IbmFormat::asFloat( const void* buf )
{
    register int fconv;
    register int fmant, t;
    fconv = *((int*)buf);

#ifdef __little__
    fconv = (fconv<<24) | ((fconv>>24)&0xff) |
	    ((fconv&0xff00)<<8) | ((fconv&0xff0000)>>8);
#endif
 
    if ( fconv )
    {
	fmant = 0x00ffffff & fconv;
	t = (int) ((0x7f000000 & fconv) >> 22) - 130;
	if ( t <= 0 ) fconv = 0;
	else
	{
	    while ( !(fmant & 0x00800000) && t != -1 ) { --t; fmant <<= 1; }
	    if (t > 254) fconv = (0x80000000 & fconv) | 0x7f7fffff;
	    else if (t <= 0) fconv = 0;
	    else fconv = (0x80000000 & fconv) |(t << 23)|(0x007fffff & fmant);
	}
    }


    return *((float*)(&fconv));
}


void IbmFormat::putFloat( float value, void* buf )
{
    register int* iconv = (int*)(&value);
    register int fmant, t, fconv = *iconv;

    if ( fconv )
    {
	fmant = (0x007fffff & fconv) | 0x00800000;
	t = (int) ((0x7f800000 & fconv) >> 23) - 126;
	while (t & 0x3) { ++t; fmant >>= 1; }
	fconv = (0x80000000 & fconv) | (((t>>2) + 64) << 24) | fmant;
    }
#ifdef __little__
    fconv = (fconv<<24) | ((fconv>>24)&0xff) |
	    ((fconv&0xff00)<<8) | ((fconv&0xff0000)>>8);
#endif

    memcpy( buf, &fconv, 4 );
}
