/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2008
-*/

static const char* rcsID = "$Id: color.cc,v 1.12 2011-04-01 10:46:50 cvsbert Exp $";

#include "color.h"

#include "separstr.h"
#include "bufstringset.h"
#include "typeset.h"
#include <stdlib.h>
#include <string.h>


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
    if ( !str ) return false;
    if ( *str == '#' )
	{ setStdStr(str); return true; }
    if ( !*str ) return false;

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


void Color::getHSV( unsigned char& h_, unsigned char& s_,
		    unsigned char& v_ ) const
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
    return c >= 'a' && c <= 'f' ? 10 + (c - 'a') : c - '0';
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


struct ColorDescriptionData
{
    int		r_, g_, b_;
    const char	*nm1_, *nm2_, *nm3_;
};

static const int cNrColDD = 135;
static const ColorDescriptionData cColDD[] = {
{	255,	235,	205,	"Almond", "Blanched", "" },
{	102,	205,	170,	"Aquamarine", "Medium", "" },
{	127,	255,	212,	"Aquamarine", "", "" },
{	240,	255,	255,	"Azure", "", "" },
{	245,	245,	220,	"Beige", "", "" },
{	255,	228,	196,	"Bisque", "", "" },
{	0,	0,	0,	"Black", "", "" },
{	0,	0,	128,	"Blue", "Navy", "" },
{	0,	0,	139,	"Blue", "Dark", "" },
{	0,	0,	205,	"Blue", "Medium", "" },
{	0,	0,	255,	"Blue", "", "" },
{	0,	191,	255,	"Blue", "Sky", "Deep" },
{	100,	149,	237,	"Blue", "Cornflower", "" },
{	106,	90,	205,	"Blue", "Slate", "" },
{	123,	104,	238,	"Blue", "Slate", "Medium" },
{	132,	112,	255,	"Blue", "Slate", "Light" },
{	135,	206,	235,	"Blue", "Sky", "" },
{	135,	206,	250,	"Blue", "Sky", "Light" },
{	173,	216,	230,	"Blue", "Light", "" },
{	176,	196,	222,	"Blue", "Steel", "Light" },
{	176,	224,	230,	"Blue", "Powder", "" },
{	240,	248,	255,	"Blue", "Alice", "" },
{	25,	25,	112,	"Blue", "Midnight", "" },
{	30,	144,	255,	"Blue", "Dodger", "" },
{	65,	105,	225,	"Blue", "Royal", "" },
{	70,	130,	180,	"Blue", "Steel", "" },
{	72,	61,	139,	"Blue", "Slate", "Dark" },
{	95,	158,	160,	"Blue", "Cadet", "" },
{	255,	240,	245,	"Blush", "Lavender", "" },
{	139,	69,	19,	"Brown", "Saddle", "" },
{	165,	42,	42,	"Brown", "", "" },
{	188,	143,	143,	"Brown", "Rosy", "" },
{	210,	105,	30,	"Brown", "Chocolate", "" },
{	244,	164,	96,	"Brown", "Sandy", "" },
{	222,	184,	135,	"Burlywood", "", "" },
{	127,	255,	0,	"Chartreuse", "", "" },
{	255,	250,	205,	"Chiffon", "Lemon", "" },
{	240,	128,	128,	"Coral", "Light", "" },
{	255,	127,	80,	"Coral", "", "" },
{	255,	248,	220,	"Cornsilk", "", "" },
{	245,	255,	250,	"Cream", "Mint", "" },
{	0,	139,	139,	"Cyan", "Dark", "" },
{	0,	255,	255,	"Cyan", "", "" },
{	224,	255,	255,	"Cyan", "Light", "" },
{	178,	34,	34,	"Firebrick", "", "" },
{	220,	220,	220,	"Gainsboro", "", "" },
{	255,	215,	0,	"Gold", "", "" },
{	184,	134,	11,	"Goldenrod", "Dark", "" },
{	218,	165,	32,	"Goldenrod", "", "" },
{	238,	221,	130,	"Goldenrod", "Light", "" },
{	238,	232,	170,	"Goldenrod", "Pale", "" },
{	0,	100,	0,	"Green", "Dark", "" },
{	0,	250,	154,	"Green", "Spring", "Medium" },
{	0,	255,	0,	"Green", "", "" },
{	0,	255,	127,	"Green", "Spring", "" },
{	124,	252,	0,	"Green", "Lawn", "" },
{	143,	188,	143,	"Green", "Sea", "Dark" },
{	144,	238,	144,	"Green", "Light", "" },
{	152,	251,	152,	"Green", "Pale", "" },
{	154,	205,	50,	"Green", "Yellow", "" },
{	32,	178,	170,	"Green", "Sea", "Light" },
{	34,	139,	34,	"Green", "Forest", "" },
{	46,	139,	87,	"Green", "Sea", "" },
{	50,	205,	50,	"Green", "Lime", "" },
{	60,	179,	113,	"Green", "Sea", "Medium" },
{	85,	107,	47,	"Green", "Olive", "Dark" },
{	105,	105,	105,	"Grey", "Dim", "" },
{	112,	128,	144,	"Grey", "Slate", "" },
{	119,	136,	153,	"Grey", "Slate", "Light" },
{	169,	169,	169,	"Grey", "Dark", "" },
{	190,	190,	190,	"Grey", "", "" },
{	211,	211,	211,	"Grey", "Light", "" },
{	47,	79,	79,	"Grey", "Slate", "Dark" },
{	240,	255,	240,	"Honeydew", "", "" },
{	255,	255,	240,	"Ivory", "", "" },
{	189,	183,	107,	"Khaki", "Dark", "" },
{	240,	230,	140,	"Khaki", "", "" },
{	253,	245,	230,	"Lace", "Old", "" },
{	230,	230,	250,	"Lavender", "", "" },
{	250,	240,	230,	"Linen", "", "" },
{	139,	0,	139,	"Magenta", "Dark", "" },
{	255,	0,	255,	"Magenta", "", "" },
{	176,	48,	96,	"Maroon", "", "" },
{	255,	228,	181,	"Moccasin", "", "" },
{	107,	142,	35,	"Olive", "", "" },
{	255,	140,	0,	"Orange", "Dark", "" },
{	255,	165,	0,	"Orange", "", "" },
{	153,	50,	204,	"Orchid", "Dark", "" },
{	186,	85,	211,	"Orchid", "Medium", "" },
{	218,	112,	214,	"Orchid", "", "" },
{	205,	133,	63,	"Peru", "", "" },
{	255,	105,	180,	"Pink", "Hot", "" },
{	255,	182,	193,	"Pink", "Light", "" },
{	255,	192,	203,	"Pink", "", "" },
{	255,	20,	147,	"Pink", "Deep", "" },
{	221,	160,	221,	"Plum", "", "" },
{	255,	218,	185,	"Puff", "Peach", "" },
{	147,	112,	219,	"Purple", "Medium", "" },
{	160,	32,	240,	"Purple", "", "" },
{	139,	0,	0,	"Red", "Dark", "" },
{	199,	21,	133,	"Red", "Violet", "Medium" },
{	205,	92,	92,	"Red", "Indian", "" },
{	208,	32,	144,	"Red", "Violet", "" },
{	219,	112,	147,	"Red", "Violet", "Pale" },
{	255,	0,	0,	"Red", "", "" },
{	255,	69,	0,	"Red", "Orange", "" },
{	255,	228,	225,	"Rose", "Misty", "" },
{	233,	150,	122,	"Salmon", "Dark", "" },
{	250,	128,	114,	"Salmon", "", "" },
{	255,	160,	122,	"Salmon", "Light", "" },
{	255,	245,	238,	"Seashell", "", "" },
{	160,	82,	45,	"Sienna", "", "" },
{	245,	245,	245,	"Smoke", "White", "" },
{	255,	250,	250,	"Snow", "", "" },
{	210,	180,	140,	"Tan", "", "" },
{	216,	191,	216,	"Thistle", "", "" },
{	255,	99,	71,	"Tomato", "", "" },
{	0,	206,	209,	"Turquoise", "Dark", "" },
{	175,	238,	238,	"Turquoise", "Pale", "" },
{	64,	224,	208,	"Turquoise", "", "" },
{	72,	209,	204,	"Turquoise", "Medium", "" },
{	138,	43,	226,	"Violet", "Blue", "" },
{	148,	0,	211,	"Violet", "Dark", "" },
{	238,	130,	238,	"Violet", "", "" },
{	245,	222,	179,	"Wheat", "", "" },
{	255,	239,	213,	"Whip", "Papaya", "" },
{	248,	248,	255,	"White", "Ghost", "" },
{	250,	235,	215,	"White", "Antique", "" },
{	255,	222,	173,	"White", "Navajo", "" },
{	255,	250,	240,	"White", "Floral", "" },
{	255,	255,	255,	"White", "", "" },
{	173,	255,	47,	"Yellow", "Green", "" },
{	250,	250,	210,	"Yellow", "Goldenrod", "Light" },
{	255,	255,	0,	"Yellow", "", "" },
{	255,	255,	224,	"Yellow", "Light", "" }
};


static const char* mkDesc( const ColorDescriptionData& cdd )
{
    static BufferString ret;
    ret.setEmpty();
    if ( *cdd.nm3_ )
	ret.add( cdd.nm3_ ).add( " " );
    if ( *cdd.nm2_ )
	ret.add( cdd.nm2_ ).add( " " );
    ret.add( cdd.nm1_ );
    return ret.buf();
}


const char* Color::getDescription() const
{
    int minsqdist = 16581375; int minidx = -1;
    for ( int idx=0; idx<cNrColDD; idx++ )
    {
	const ColorDescriptionData& cdd = cColDD[idx];
	const int rdist = r() - cdd.r_;
	const int gdist = g() - cdd.g_;
	const int bdist = b() - cdd.b_;
	const int distsq = rdist*rdist + gdist*gdist + bdist*bdist;
	if ( distsq < 4 )
	    return mkDesc( cdd );
	else if ( distsq < minsqdist )
	    { minsqdist = distsq; minidx = idx; }
    }

    return mkDesc( cColDD[minidx] );
}


bool Color::fromDescription( const char* inp )
{
    if ( !inp || !*inp )
	return false;

    for ( int idx=0; idx<cNrColDD; idx++ )
    {
	const ColorDescriptionData& cdd = cColDD[idx];
	// Let's test whether inp starts with the right character:
	if ( *cdd.nm3_ )
	    { if ( *inp != *cdd.nm3_ ) continue; }
	else if ( *cdd.nm2_ )
	    { if ( *inp != *cdd.nm2_ ) continue; }
	else
	    { if ( *inp != *cdd.nm1_ ) continue; }

	const BufferString cddesc( mkDesc(cdd) );
	if ( cddesc == inp )
	    { *this = Color( cdd.r_, cdd.g_, cdd.b_ ); return true; }
    }

    return false;
}


const BufferStringSet& Color::descriptions()
{
    static BufferStringSet* bss = 0;
    if ( !bss )
    {
	bss = new BufferStringSet;
	for ( int idx=0; idx<cNrColDD; idx++ )
	    bss->add( mkDesc(cColDD[idx]) );
    }
    return *bss;
}


const TypeSet<Color>& Color::descriptionCenters()
{
    static TypeSet<Color>* cols = 0;
    if ( !cols )
    {
	cols = new TypeSet<Color>;
	for ( int idx=0; idx<cNrColDD; idx++ )
	{
	    const ColorDescriptionData& cdd = cColDD[idx];
	    *cols += Color( cdd.r_, cdd.g_, cdd.b_ );
	}
    }
    return *cols;
}


const char* Color::largeUserInfoString() const
{
    static BufferString ret;

    ret = getStdStr();

    ret.add( " (" ).add( getDescription() ).add( ")" )
       .add( " RGB=" )
       .add((int)r()).add("|").add((int)g()).add("|").add((int)b());

    unsigned char ch, cs, cv; getHSV( ch, cs, cv );
    ret.add( ", HSV=" )
       .add((int)ch).add("|").add((int)cs).add("|").add((int)cv);

    if ( t() )
    {
	const int promille = (int)( (t() / 0.00255) + .5 );
	ret.add( ". Transparency=" ).add( promille/10 );
	if ( promille % 10 )
	    ret.add( "." ).add( promille % 10 );
	ret.add( "%" );
    }

    return ret.buf();
}
