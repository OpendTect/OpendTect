/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: color.cc,v 1.8 2010-10-14 09:58:06 cvsbert Exp $";

#include "color.h"

#include "separstr.h"
#include "stdlib.h"
#include "string.h"


Color::Color( unsigned char r_, unsigned char g_,
	      unsigned char b_, unsigned char t_ )
{ set( r_, g_, b_, t_ ); }


Color::Color( unsigned int rgbval )
{ col_ = rgbval; }

bool Color::operator ==( const Color& c ) const
{ return col_ == c.col_; }


bool Color::operator !=( const Color& c ) const
{ return col_ != c.col_; }

unsigned char Color::r() const
{ return (unsigned char)((col_ >> 16) & 0xff); }


unsigned char Color::g() const
{ return (unsigned char)((col_ >> 8) & 0xff); }


unsigned char Color::b() const
{ return (unsigned char)(col_ & 0xff); }


unsigned char Color::t() const
{ return (unsigned char)((col_ >> 24) & 0xff); }

bool Color::isVisible() const	{ return t() < 255; }

unsigned int Color::rgb() const
{ return col_; }


unsigned int& Color::rgb() 
{ return col_; }


void Color::set( unsigned char r_, unsigned char g_,
			 unsigned char b_, unsigned char t_ )
{
    col_ = ( (unsigned int)(t_&0xff) << 24 )
	    | ( (unsigned int)(r_&0xff) << 16 )
	    | ( (unsigned int)(g_&0xff) <<  8 )
	    |		      (b_&0xff);
}


Color Color::complementaryColor() const
{ return Color(255-r(), 255-g(), 255-b(), t() ); }


Color Color::operator*( float f ) const
{
    Color res = *this;
    res.lighter( f );
    return res;
}


void Color::lighter( float f )
{
    if ( f < 0 ) f = -f;
    set( getUChar(r()*f), getUChar(g()*f),
	 getUChar(b()*f) );
}

void Color::setRgb( unsigned int rgb_  )
{ col_ = rgb_; }


void Color::setTransparency( unsigned char t_ )
{ set( r(), g(), b(), t_ ); }


unsigned char Color::getUChar( float v )
{ return v > 254.5 ? 255 : (v < 0.5 ? 0 : (unsigned char)(v+.5)); }

const int nrstddrawcols = 10;
Color stddrawcols[] = {
	Color( 220,  50,  50 ), // red
	Color(  50,  50, 220 ), // blue
	Color(  50, 200,  50 ), // green
	Color(  50, 200, 200 ), // cyan
	Color( 255, 210,   0 ), // gold
	Color( 220,   0, 220 ), // magenta
	Color( 140, 130,  80 ), // khaki
	Color( 100, 160,   0 ), // orange
	Color( 140,  35,  80 ), // dark violet red
	Color( 204, 133,  61 ), // peru
};


int Color::nrStdDrawColors()
{
    return nrstddrawcols;
}


Color Color::stdDrawColor( int idx )
{
    return stddrawcols[ idx % nrstddrawcols ];
}


void Color::fill( char* str ) const
{
    FileMultiString fms;
    fms += (int)r();
    fms += (int)g();
    fms += (int)b();
    if ( t() ) fms += (int)t();
    strcpy( str, (const char*)fms );
}


bool Color::use( const char* str )
{
    if ( !str || !*str ) return false;

    const FileMultiString fms( str );
    const int sz = fms.size();
    if ( sz < 3 ) return false;

    unsigned char r_ = (unsigned char)toInt( fms[0] );
    unsigned char g_ = (unsigned char)toInt( fms[1] );
    unsigned char b_ = (unsigned char)toInt( fms[2] );
    unsigned char t_ = sz > 3 ? (unsigned char)toInt( fms[3] ) : 0;
    set( r_, g_, b_, t_ );

    return true;
}


void Color::getHSV( unsigned char& h_, unsigned char& s_, unsigned char& v_ )
{
    int r_ = (int)r();
    int g_ = (int)g();
    int b_ = (int)b();
    
    float fr = (float)r_ / 255;
    float fg = (float)g_ / 255;
    float fb = (float)b_ / 255;

    float min = fr; float max = fr; int maxid = 0;
    if ( fg > max ) { max = fg; maxid = 1; }
    if ( fg < min ) { min = fg; }
    if ( fb > max ) { max = fb; maxid = 2; }
    if ( fb < min ) { min = fb; }

    float h, s, v = max;
    float delta = max - min;

    if ( !delta )
    {
        // r == g == b
        h_ = (unsigned char)0;
        s_ = (unsigned char)0;
        v_ = (unsigned char)mNINT(v*255);
        return;
    }

    s = delta / max;

    switch ( maxid )
    {
        case 0:
            h = (fg-fb)/delta; break;
        case 1:
            h = 2 + (fb-fr)/delta; break;
        case 2:
            h = 4 + (fr-fg)/delta; break;
    }

    h *= 60;                            // degrees
    if ( h < 0 )
        h += 360;

    h_ = (unsigned char)mNINT(h);
    s_ = (unsigned char)mNINT(s*255);
    v_ = (unsigned char)mNINT(v*255);
}


void Color::setHSV( unsigned char h_, unsigned char s_, unsigned char v_ )
{
    unsigned char r_, g_, b_;
    if ( (int)s_ == 0 )
    {
        // achromatic (grey)
        r_ = g_ = b_ = v_;
	set( r_, g_, b_, t() );
        return;
    }

    float h = (float)h_ / 60;
    float s = (float)s_ / 255;
    float v = (float)v_ / 255;
    float fr, fg, fb;

    int i = (int)h;
    float f = h - i;                    // factorial part of h
    float p = v * ( 1 - s );
    float q = v * ( 1 - s * f );
    float u = v * ( 1 - s * ( 1 - f ) );

    switch( i )
    {
        case 0:
            fr = v; fg = u; fb = p; break;
        case 1:
            fr = q; fg = v; fb = p; break;
        case 2:
            fr = p; fg = v; fb = u; break;
        case 3:
            fr = p; fg = q; fb = v; break;
        case 4:
            fr = u; fg = p; fb = v; break;
        case 5:
            fr = v; fg = p; fb = q; break;
    }

    r_ = (unsigned char)mNINT(fr*255);
    g_ = (unsigned char)mNINT(fg*255);
    b_ = (unsigned char)mNINT(fb*255);
    set( r_, g_, b_, t() );
}


unsigned char fromHexVal( char c )
{
    return c >= 'a' || c <= 'f' ? 10 + (c - 'a') : c - '0';
}


unsigned char getCompFromStrPart( const char* str )
{
    if ( !str || !*str ) return 255;
    unsigned char c1 = fromHexVal( *str );
    unsigned char c2 = *(str+1) ? fromHexVal(*(str+1)) : 0;
    return 16 * c1 + c2;
}


void Color::setStdStr( const char* str )
{
    if ( !str || !*str ) return;
    if ( *str == '#' ) str++;
    const int len = strlen(str);

    unsigned char r_ = getCompFromStrPart( str );
    unsigned char g_ = len > 2 ? getCompFromStrPart(str+2) : 255;
    unsigned char b_ = len > 4 ? getCompFromStrPart(str+4) : 255;
    unsigned char t_ = len > 6 ? getCompFromStrPart(str+6) : 255;

    set( r_, g_, b_, len > 6 ? t_ : t() );
}


static char toHexVal( unsigned char c )
{
    return c < 10 ? '0' + c : 'a' + (c-10);
}


static void addCompToStr( char* str, unsigned char comp )
{
    *str = toHexVal( comp / 16 );
    *(str+1) = toHexVal( comp % 16 );
}


static void addTranspToStr( char* str, unsigned char val, int transpopt,
			    int& curidx )
{
    if ( transpopt == 0 ) return;

    if ( transpopt < 0 )
	val = 255 - val;
    addCompToStr( str+curidx, val ); curidx += 2;
}


const char* Color::getStdStr( bool withhash, int transpopt ) const
{
    static char buf[10];
    int curidx = 0;
    const bool isrev = !withhash;
    if ( withhash ) { buf[curidx] = '#'; curidx++; }
    if ( isrev ) addTranspToStr( buf, t(), transpopt, curidx );
    addCompToStr( buf+curidx, withhash ? r() : b() ); curidx += 2;
    addCompToStr( buf+curidx, g() ); curidx += 2;
    addCompToStr( buf+curidx, withhash ? b() : r() ); curidx += 2;
    if ( !isrev ) addTranspToStr( buf, t(), transpopt, curidx );
    buf[curidx] = '\0';
    return buf;
}
