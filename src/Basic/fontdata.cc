/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "fontdata.h"
#include "separstr.h"


DefineEnumNames(FontData,Weight,2,"Font weight")
{ "Light", "Normal", "Demi-Bold", "Bold", "Black", 0 };

const char* universalfamilies[] =
{ "Helvetica", "Courier", "Times", 0 };

const char* defaultkeys[] =
{ "Control",
  "Graphics medium", "Graphics small", "Graphics large",
  "Small control", "Large control", "Fixed width", 0 };

static const int numwghts[] =
{ 25, 50, 63, 75, 87, 0 };

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
static int defaultpointsize = 12;
static FontData::Weight defaultweight = FontData::Bold;
static bool defaultitalic = false;

const char* FontData::defaultFamily()		{ return defaultfamily; }
int FontData::defaultPointSize()		{ return defaultpointsize; }
FontData::Weight FontData::defaultWeight()	{ return defaultweight; }
bool FontData::defaultItalic() 			{ return defaultitalic; }

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
    if ( nr > 1 ) pointsize_ = toInt( fms[1] );
    if ( nr > 2 ) parseEnumWeight( fms[2], weight_ );
    if ( nr > 3 ) italic_ = toBool(fms[3],false);
    
    return true;
}


void FontData::putTo( BufferString& s ) const
{
    FileMultiString fms;
    fms += family_;
    fms += pointsize_;
    fms += FontData::getWeightString(weight_);
    fms += getYesNoString( italic_ );
    s = fms;
}
