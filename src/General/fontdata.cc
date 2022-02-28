/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/


#include "fontdata.h"
#include "separstr.h"


mDefineEnumUtils(FontData,Weight,"Font weight")
{ "Light", "Normal", "Demi-Bold", "Bold", "Black", 0 };

const char* universalfamilies[] =
{ "Helvetica", "Courier", "Times", 0 };

const char* defaultkeys[] =
{ "Control",
  "Graphics 2D",
  "Graphics 3D",
  "Fixed width",
  "Graphics 2D small", "Graphics 2D large",
  "Small control", "Large control", 0 };

static const int numwghts[] =
{ 25, 50, 63, 75, 87, 0 };


FontData::FontData( int ptsz, const char* fam, Weight wght, bool ital )
    : family_(fam)
    , pointsize_(ptsz)
    , weight_(wght)
    , italic_(ital)
{
}


FontData::FontData( const char* fms )
{
    getFrom( fms );
}


bool FontData::operator ==( const FontData& oth ) const
{
    if ( &oth == this )
	return true;

    return family_ == oth.family_ && pointsize_ == oth.pointsize_ &&
	   weight_ == oth.weight_ && italic_ == oth.italic_ &&
	   styleName() == oth.styleName();
}


bool FontData::operator !=( const FontData& oth ) const
{
    return !(*this == oth);
}


FontData& FontData::operator =( const FontData& fd )
{
    family_ = fd.family_;
    pointsize_ = fd.pointsize_;
    weight_ = fd.weight_;
    italic_ = fd.italic_;
    setStyleName( fd.styleName() );
    return *this;
}


int FontData::numWeight( FontData::Weight w )
{ return numwghts[(int)w]; }


FontData::Weight FontData::enumWeight( int w )
{
    int idx = 0;
    while ( numwghts[idx] && numwghts[idx] < w ) idx++;
    if ( !numwghts[idx] ) idx--;
    return (FontData::Weight)idx;
}


const char* const* FontData::universalFamilies() { return universalfamilies; }
const char* const* FontData::defaultKeys()	 { return defaultkeys; }
const char* FontData::key( StdSz ss )		 { return defaultkeys[(int)ss];}


// static variables and their access functions
static BufferString defaultfamily( universalfamilies[0] );
static int defaultpointsize = 10;
static FontData::Weight defaultweight = FontData::Normal;
static bool defaultitalic = false;

const char* FontData::defaultFamily()		{ return defaultfamily; }
int FontData::defaultPointSize()		{ return defaultpointsize; }
FontData::Weight FontData::defaultWeight()	{ return defaultweight; }
bool FontData::defaultItalic()			{ return defaultitalic; }

void FontData::setDefaultFamily( const char* f)	{ defaultfamily = f; }
void FontData::setDefaultPointSize( int ps )	{ defaultpointsize = ps; }
void FontData::setDefaultWeight( Weight w )	{ defaultweight = w; }
void FontData::setDefaultItalic( bool yn )	{ defaultitalic = yn; }


void FontData::setStyleName( const char* stylenm )
{
    stylename_ = stylenm;
}


const char* FontData::styleName() const
{
    return stylename_.buf();
}


bool FontData::getFrom( const char* s )
{
    FileMultiString fms( s );
    const int nr = fms.size();
    if ( nr < 1 ) return false;

    family_ = fms[0];
    if ( nr > 1 ) pointsize_ = fms.getIValue( 1 );
    if ( nr > 2 ) parseEnumWeight( fms[2], weight_ );
    if ( nr > 3 ) italic_ = toBool(fms[3],false);
    if ( nr > 4 ) setStyleName( fms[4] );

    return true;
}


void FontData::putTo( BufferString& s ) const
{
    FileMultiString fms;
    fms += family_;
    fms += pointsize_;
    fms += FontData::getWeightString(weight_);
    fms += getYesNoString( italic_ );
    fms += styleName();
    s = fms;
}
