/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2001
-*/


#include "fontdata.h"
#include "separstr.h"


mDefineEnumUtils(FontData,Weight,"Font weight")
{ "Light", "Normal", "Demi-Bold", "Bold", "Black", 0 };
template<>
void EnumDefImpl<FontData::Weight>::init()
{
    uistrings_ += mEnumTr("Light","Font Weight");
    uistrings_ += mEnumTr("Normal","Font Weight");
    uistrings_ += mEnumTr("Demi-Bold","Font Weight");
    uistrings_ += mEnumTr("Bold","Font Weight");
    uistrings_ += mEnumTr("Black","Font Weight");
}

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
    : family_(defaultFamily())
    , pointsize_(defaultPointSize())
    , weight_(defaultWeight())
    , italic_(defaultItalic())
{
    getFrom( fms );
}


bool FontData::operator ==( const FontData& oth ) const
{
    BufferString myser, othser; putTo( myser ); oth.putTo( othser );
    return myser == othser;
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
static int default3dpointsize = 16;
static FontData::Weight defaultweight = FontData::Normal;
static bool defaultitalic = false;

const char* FontData::defaultFamily()		{ return defaultfamily; }
int FontData::defaultPointSize()		{ return defaultpointsize; }
int FontData::default3DPointSize()		{ return default3dpointsize; }
FontData::Weight FontData::defaultWeight()	{ return defaultweight; }
bool FontData::defaultItalic()			{ return defaultitalic; }

void FontData::setDefaultFamily( const char* f)	{ defaultfamily = f; }
void FontData::setDefaultPointSize( int ps )	{ defaultpointsize = ps; }
void FontData::setDefaultWeight( Weight w )	{ defaultweight = w; }
void FontData::setDefaultItalic( bool yn )      { defaultitalic = yn; }



bool FontData::getFrom( const char* s )
{
    FileMultiString fms( s );
    const int nr = fms.size();
    if ( nr < 1 ) return false;

    family_ = fms[0];
    if ( nr > 1 ) pointsize_ = fms.getIValue( 1 );
    if ( nr > 2 ) WeightDef().parse( fms[2], weight_ );
    if ( nr > 3 ) italic_ = toBool(fms[3],false);

    return true;
}


void FontData::putTo( BufferString& s ) const
{
    FileMultiString fms;
    fms += family_;
    fms += pointsize_;
    fms += FontData::toString(weight_);
    fms += getYesNoString( italic_ );
    s = fms;
}
