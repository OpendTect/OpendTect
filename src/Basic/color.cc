/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2008
-*/


#include "color.h"

#include "bufstringset.h"
#include "math2.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "separstr.h"
#include "typeset.h"


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
{
    return Color(255-r(), 255-g(), 255-b(), t() );
}


TypeSet<Color> Color::complimentaryColors( int numColor ) const
{
    if ( numColor<1 )
	numColor = 1;
    TypeSet<Color> res( numColor, Color() );
    unsigned char h, v, s;
    getHSV( h, s, v );
    s = 255-s;
    v = 255-v;
    int hdelta = mNINT32(60.0/(float)(numColor+1));
    int hs = h;
    Color tmp;
    for ( int idx=0; idx<numColor; idx++ )
    {
	hs += (idx+1)*hdelta;
	h = hs%60;
	tmp.setHSV( h, s, v );
	res[idx] = tmp;
    }
    return res;
}


Color Color::lighter( float fac ) const
{
    if ( fac == 0 )
	return Color( *this );

    float newr, newg, newb;
    if ( fac > 0 )
    {
	newr = 255 - ((255.0f - r()) / (fac + 1));
	newg = 255 - ((255.0f - g()) / (fac + 1));
	newb = 255 - ((255.0f - b()) / (fac + 1));
    }
    else
    {
	newr = r() / (1 - fac);
	newg = g() / (1 - fac);
	newb = b() / (1 - fac);
    }

    return Color( getUChar(newr), getUChar(newg), getUChar(newb) );
}


float Color::contrast( const Color& c ) const
{
    float L1 = getRelLuminance();
    float L2 = c.getRelLuminance();
    return L1>L2 ? (L1 + 0.05) / (L2 + 0.05) : (L2 + 0.05) / (L1 + 0.05);
}


float Color::getRelLuminance() const
{
    const float Rg = r()<=10 ? (float)r()/3294.0
			     : Math::PowerOf((float)r()/269.0 + 0.0513,2.4);
    const float Gg = g()<=10 ? (float)g()/3294.0
			     : Math::PowerOf((float)g()/269.0 + 0.0513,2.4);
    const float Bg = b()<=10 ? (float)b()/3294.0
			     : Math::PowerOf((float)b()/269.0 + 0.0513,2.4);

    return 0.2126 * Rg + 0.7152 * Gg + 0.0722 * Bg;
}

void Color::setRgb( unsigned int rgb_  )
{ col_ = rgb_; }


void Color::setTransparency( unsigned char t_ )
{ set( r(), g(), b(), t_ ); }


void Color::setTransparencyF( float tf )
{ setTransparency( mCast(unsigned char,(tf*255.0f)) ); }


unsigned char Color::getUChar( float v )
{ return v > 254.5 ? 255 : (v < 0.5 ? 0 : (unsigned char)(v+.5)); }

float Color::getFloat( unsigned char c )
{ return (mCast(float,c)/255.0f); }

const int nrstddrawcols = 10;
Color stddrawcols[] = {
	Color( 220,  20,  60 ), // Crimson
	Color(  65, 105, 225 ), // RoyalBlue
	Color(  50, 205,  50 ), // LimeGreen
	Color(  72, 209, 204 ), // MediumTurquoise
	Color( 255, 215,   0 ), // Gold
	Color( 148,   0, 211 ), // DarkViolet
	Color( 128, 128,   0 ), // Olive
	Color( 255, 140,   0 ), // DarkOrange
	Color(  72,  61, 139 ), // DarkSlateBlue
	Color( 205, 133,  63 ), // Peru
};


int Color::nrStdDrawColors()
{
    return nrstddrawcols;
}


Color Color::stdDrawColor( int idx )
{
    return stddrawcols[ idx % nrstddrawcols ];
}


Color Color::interpolate( const Color& col1, const Color& col2, float frac )
{
    return Color( getUChar((col2.r() - col1.r())*frac + col1.r()),
		  getUChar((col2.g() - col1.g())*frac + col1.g()),
		  getUChar((col2.b() - col1.b())*frac + col1.b()),
		  getUChar((col2.t() - col1.t())*frac + col1.t()) );
}


void Color::fill( BufferString& bs ) const
{
    FileMultiString fms;
    fms += (int)r();
    fms += (int)g();
    fms += (int)b();
    if ( t() ) fms += (int)t();
    bs = fms.buf();
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

    unsigned char r_ = (unsigned char)fms.getIValue( 0 );
    unsigned char g_ = (unsigned char)fms.getIValue( 1 );
    unsigned char b_ = (unsigned char)fms.getIValue( 2 );
    unsigned char t_ = sz > 3 ? (unsigned char)fms.getIValue( 3 ) : 0;
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
        v_ = (unsigned char)mNINT32(v*255);
        return;
    }

    s = delta / max;

    switch ( maxid )
    {
        case 0:
            h = (fg-fb)/delta; break;
        case 1:
            h = 2 + (fb-fr)/delta; break;
        default: // 2
            h = 4 + (fr-fg)/delta; break;
    }

    h *= 60;                            // degrees
    if ( h < 0 )
        h += 360;

    h_ = (unsigned char)mNINT32(h);
    s_ = (unsigned char)mNINT32(s*255);
    v_ = (unsigned char)mNINT32(v*255);
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
	default:
            fr = v; fg = p; fb = q; break;
    }

    r_ = (unsigned char)mNINT32(fr*255);
    g_ = (unsigned char)mNINT32(fg*255);
    b_ = (unsigned char)mNINT32(fb*255);
    set( r_, g_, b_, t() );
}


static unsigned char fromHexVal( char c )
{
    return c >= 'a' && c <= 'f' ? 10 + (c - 'a') : c - '0';
}


static unsigned char getCompFromStrPart( const char* str )
{
    if ( !str || !*str ) return 255;
    unsigned char c1 = fromHexVal( *str );
    unsigned char c2 = *(str+1) ? fromHexVal(*(str+1)) : 0;
    return 16 * c1 + c2;
}


void Color::setStdStr( const char* str )
{
    FixedString inp( str );
    if ( inp.isEmpty() )
	return;

    const bool isnorm = inp.firstChar() == '#';
    if ( isnorm ) inp = str + 1;
    const int len = inp.size();
    if ( len < 6 )
	return;

    if ( isnorm ) str++;
    unsigned char r_ = getCompFromStrPart( str );
    unsigned char g_ = len > 2 ? getCompFromStrPart(str+2) : 255;
    unsigned char b_ = len > 4 ? getCompFromStrPart(str+4) : 255;
    unsigned char t_ = len > 6 ? getCompFromStrPart(str+6) : 255;

    if ( !isnorm )
    {
	if ( len > 6 )
	    { Swap( r_, t_ ); Swap( g_, b_ ); }
	else if ( len > 4 )
	    Swap( r_, b_ );
	else if ( len > 2 )
	    Swap( r_, g_ );
    }

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
    mDefineStaticLocalObject( char, buf, [10] );
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


void Color::convertToStr( const TypeSet<Color>& cols, BufferStringSet& strs )
{
    for ( int idx=0; idx<cols.size(); idx++ )
	strs.add( cols[idx].getStdStr() );
}


void Color::convertFromStr( const BufferStringSet& strs, TypeSet<Color>& cols )
{
    Color col;
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	col.setStdStr( strs.get(idx) );
	cols += col;
    }
}


struct ColorDescriptionData
{
    int		r_, g_, b_;
    const char*	nm_;
};


// These are the standard HTML colors
static const int cNrColDD = 139;
static const ColorDescriptionData cColDD[] = {
{	240,	248,	255,	"AliceBlue" },
{	50,	50,	50,	"Anthracite" },
{	250,	235,	215,	"AntiqueWhite" },
{	127,	255,	212,	"Aquamarine" },
{	240,	255,	255,	"Azure" },
{	245,	245,	220,	"Beige" },
{	255,	228,	196,	"Bisque" },
{	0,	0,	0,	"Black" },
{	255,	235,	205,	"BlanchedAlmond" },
{	0,	0,	255,	"Blue" },
{	138,	43,	226,	"BlueViolet" },
{	165,	42,	42,	"Brown" },
{	222,	184,	135,	"Burlywood" },
{	95,	158,	160,	"CadetBlue" },
{	127,	255,	0,	"Chartreuse" },
{	210,	105,	30,	"Chocolate" },
{	255,	127,	80,	"Coral" },
{	100,	149,	237,	"CornflowerBlue" },
{	255,	248,	220,	"Cornsilk" },
{	220,	20,	60,	"Crimson" },
{	0,	255,	255,	"Cyan" },
{	0,	0,	139,	"DarkBlue" },
{	0,	139,	139,	"DarkCyan" },
{	184,	134,	11,	"DarkGoldenrod" },
{	0,	100,	0,	"DarkGreen" },
{	169,	169,	169,	"DarkGrey" },
{	189,	183,	107,	"DarkKhaki" },
{	139,	0,	139,	"DarkMagenta" },
{	85,	107,	47,	"DarkOliveGreen" },
{	255,	140,	0,	"DarkOrange" },
{	153,	50,	204,	"DarkOrchid" },
{	139,	0,	0,	"DarkRed" },
{	233,	150,	122,	"DarkSalmon" },
{	143,	188,	143,	"DarkSeaGreen" },
{	72,	61,	139,	"DarkSlateBlue" },
{	47,	79,	79,	"DarkSlateGrey" },
{	0,	206,	209,	"DarkTurquoise" },
{	148,	0,	211,	"DarkViolet" },
{	255,	20,	147,	"DeepPink" },
{	0,	191,	255,	"DeepSkyBlue" },
{	105,	105,	105,	"DimGrey" },
{	30,	144,	255,	"DodgerBlue" },
{	178,	34,	34,	"Firebrick" },
{	255,	250,	240,	"FloralWhite" },
{	34,	139,	34,	"ForestGreen" },
{	220,	220,	220,	"Gainsboro" },
{	248,	248,	255,	"GhostWhite" },
{	255,	215,	0,	"Gold" },
{	218,	165,	32,	"Goldenrod" },
{	0,	128,	0,	"Green" },
{	173,	255,	47,	"GreenYellow" },
{	128,	128,	128,	"Grey" },
{	240,	255,	240,	"Honeydew" },
{	255,	105,	180,	"HotPink" },
{	205,	92,	92,	"IndianRed" },
{	75,	0,	130,	"Indigo" },
{	255,	255,	240,	"Ivory" },
{	240,	230,	140,	"Khaki" },
{	230,	230,	250,	"Lavender" },
{	255,	240,	245,	"LavenderBlush" },
{	124,	252,	0,	"LawnGreen" },
{	255,	250,	205,	"LemonChiffon" },
{	173,	216,	230,	"LightBlue" },
{	240,	128,	128,	"LightCoral" },
{	224,	255,	255,	"LightCyan" },
{	250,	250,	210,	"LightGoldenrodYellow" },
{	144,	238,	144,	"LightGreen" },
{	211,	211,	211,	"LightGrey" },
{	255,	182,	193,	"LightPink" },
{	255,	160,	122,	"LightSalmon" },
{	32,	178,	170,	"LightSeaGreen" },
{	135,	206,	250,	"LightSkyBlue" },
{	119,	136,	153,	"LightSlateGrey" },
{	176,	196,	222,	"LightSteelBlue" },
{	255,	255,	224,	"LightYellow" },
{	0,	255,	0,	"Lime" },
{	50,	205,	50,	"LimeGreen" },
{	250,	240,	230,	"Linen" },
{	255,	0,	255,	"Magenta" },
{	128,	0,	0,	"Maroon" },
{	102,	205,	170,	"MediumAquamarine" },
{	0,	0,	205,	"MediumBlue" },
{	186,	85,	211,	"MediumOrchid" },
{	147,	112,	219,	"MediumPurple" },
{	60,	179,	113,	"MediumSeaGreen" },
{	123,	104,	238,	"MediumSlateBlue" },
{	0,	250,	154,	"MediumSpringGreen" },
{	72,	209,	204,	"MediumTurquoise" },
{	199,	21,	133,	"MediumVioletRed" },
{	25,	25,	112,	"MidnightBlue" },
{	245,	255,	250,	"Mintcream" },
{	255,	228,	225,	"MistyRose" },
{	255,	228,	181,	"Moccasin" },
{	255,	222,	173,	"NavajoWhite" },
{	0,	0,	128,	"Navy" },
{	253,	245,	230,	"OldLace" },
{	128,	128,	0,	"Olive" },
{	107,	142,	35,	"OliveDrab" },
{	255,	165,	0,	"Orange" },
{	255,	69,	0,	"OrangeRed" },
{	218,	112,	214,	"Orchid" },
{	238,	232,	170,	"PaleGoldenrod" },
{	152,	251,	152,	"PaleGreen" },
{	175,	238,	238,	"PaleTurquoise" },
{	219,	112,	147,	"PaleVioletRed" },
{	255,	239,	213,	"Papayawhip" },
{	255,	218,	185,	"Peachpuff" },
{	205,	133,	63,	"Peru" },
{	255,	192,	203,	"Pink" },
{	221,	160,	221,	"Plum" },
{	176,	224,	230,	"PowderBlue" },
{	128,	0,	128,	"Purple" },
{	255,	0,	0,	"Red" },
{	188,	143,	143,	"RosyBrown" },
{	65,	105,	225,	"RoyalBlue" },
{	139,	69,	19,	"SaddleBrown" },
{	250,	128,	114,	"Salmon" },
{	244,	164,	96,	"SandyBrown" },
{	46,	139,	87,	"SeaGreen" },
{	255,	245,	238,	"Seashell" },
{	160,	82,	45,	"Sienna" },
{	192,	192,	192,	"Silver" },
{	135,	206,	235,	"SkyBlue" },
{	106,	90,	205,	"SlateBlue" },
{	112,	128,	144,	"SlateGrey" },
{	255,	250,	250,	"Snow" },
{	0,	255,	127,	"SpringGreen" },
{	70,	130,	180,	"SteelBlue" },
{	210,	180,	140,	"Tan" },
{	0,	128,	128,	"Teal" },
{	216,	191,	216,	"Thistle" },
{	255,	99,	71,	"Tomato" },
{	64,	224,	208,	"Turquoise" },
{	238,	130,	238,	"Violet" },
{	245,	222,	179,	"Wheat" },
{	255,	255,	255,	"White" },
{	245,	245,	245,	"WhiteSmoke" },
{	255,	255,	0,	"Yellow" },
{	154,	205,	50,	"YellowGreen" }
};


static const char* getApproxDesc( const char* nm )
{
    mDeclStaticString( ret );
    ret = "~";
    ret += nm;
    return ret.buf();
}


const char* Color::getDescription() const
{
    int minsqdist = 16581376; int minidx = -1;
    for ( int idx=0; idx<cNrColDD; idx++ )
    {
	const ColorDescriptionData& cdd = cColDD[idx];
	const int rdist = r() - cdd.r_;
	const int gdist = g() - cdd.g_;
	const int bdist = b() - cdd.b_;
	const int distsq = rdist*rdist + gdist*gdist + bdist*bdist;
	if ( distsq < 4 )
	    return distsq ? getApproxDesc(cdd.nm_) : cdd.nm_;
	else if ( distsq < minsqdist )
	    { minsqdist = distsq; minidx = idx; }
    }

    return getApproxDesc( cColDD[minidx].nm_ );
}


bool Color::fromDescription( const char* inp )
{
    if ( !inp || !*inp )
	return false;
    if ( *inp == '~' )
	inp++;
    if ( !*inp )
	return false;

    for ( int idx=0; idx<cNrColDD; idx++ )
    {
	const ColorDescriptionData& cdd = cColDD[idx];
	if ( caseInsensitiveEqual(cdd.nm_,inp) )
	{
	    *this = Color( (unsigned char) cdd.r_,
			   (unsigned char) cdd.g_,
			   (unsigned char) cdd.b_ );
	    return true;
	}
    }

    return false;
}


static BufferStringSet* descCreator()
{
    BufferStringSet* newbss = new BufferStringSet;
    for ( int idx=0; idx<cNrColDD; idx++ )
	newbss->add( cColDD[idx].nm_ );

    return newbss;
}

static PtrMan<BufferStringSet> descs = 0;


const BufferStringSet& Color::descriptions()
{ return *descs.createIfNull( descCreator ); }

static PtrMan<TypeSet<Color> > cols = 0;

static TypeSet<Color>* descCenterCreator()
{
    TypeSet<Color>* newcols = new TypeSet<Color>;
    for ( int idx=0; idx<cNrColDD; idx++ )
    {
	const ColorDescriptionData& cdd = cColDD[idx];
	*newcols += Color( (unsigned char) cdd.r_,
			(unsigned char) cdd.g_,
			(unsigned char) cdd.b_ );
    }

    return newcols;
}


const TypeSet<Color>& Color::descriptionCenters()
{ return *cols.createIfNull( descCenterCreator ); }


const char* Color::largeUserInfoString() const
{
    mDeclStaticString( ret );

    ret = getStdStr();

    ret.add( " (" ).add( getDescription() ).add( ")" )
       .add( " RGB=" )
       .add((int)r()).add("|").add((int)g()).add("|").add((int)b());

    unsigned char ch, cs, cv; getHSV( ch, cs, cv );
    ret.add( ", HSV=" )
       .add((int)ch).add("|").add((int)cs).add("|").add((int)cv);

    if ( t() )
    {
	const int promille = (int)( (t() / 0.255) + .5 );
	ret.add( ". Transparency=" ).add( promille/10 );
	if ( promille % 10 )
	    ret.add( "." ).add( promille % 10 );
	ret.add( "%" );
    }

    return ret.buf();
}
