/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-4-1996
-*/

static const char* rcsID = "$Id: draw.cc,v 1.84 2012/09/17 08:42:08 cvsbert Exp $";

/*! \brief Several implementations for UI-related things.

The main chunk is color table related.
*/

#include "draw.h"
#include "separstr.h"
#include "bufstringset.h"
#include "iopar.h"

// Beware if you add or change: there is a 'mirror' in
// uiGraphicsItem::setFillPattern. Not an enum to keep full flexibility.
static const int cNoFillPatternType = 0;
static const int cDotsFillPatternType = 1;
static const int cLinesFillPatternType = 2;

DefineEnumNames(Alignment,HPos,1,"Alignment")
{ "Left", "Right", "Center", 0 };
DefineEnumNames(Alignment,VPos,1,"Alignment")
{ "Top", "Bottom", "Center", 0 };
DefineEnumNames(MarkerStyle2D,Type,2,"Marker type")
{ "None", "Square", "Circle", "Cross", "Plus", "Target",
  "Horizontal line", "Vertical line", 0 };
DefineEnumNames(MarkerStyle3D,Type,0,"Marker type")
{ "None", "Cube", "Cone", "Cylinder", "Sphere", "Arrow", "Cross", "Point", "Plane", 0 };
DefineEnumNames(LineStyle,Type,0,"Line style")
{ "None", "Solid", "Dashed", "Dotted", "Dash-Dotted", "Dash-Dot-Dotted",0 };

Alignment::Alignment( HPos h, VPos v )
    : hor_(h), ver_(v)                                  {}
Alignment::Alignment( Pos h, Pos v )
    : hor_(h==Start?Left:(h==Stop?Right:HCenter))
    , ver_(v==Start?Top:(v==Stop?Bottom:VCenter))       {}


Alignment::HPos Alignment::opposite( HPos p )
{ return p == Left ? Right : (p == Right ? Left : HCenter); }


Alignment::VPos Alignment::opposite( VPos p )
{ return p == Top ? Bottom : (p == Bottom ? Top : VCenter); }

Alignment::Pos Alignment::pos( bool hor ) const
{
    if ( hor )
	return hor_ == Left ? Start : (hor_ == Right ? Stop : Center);
    return ver_ == Top ? Start : (ver_ == Bottom ? Stop : Center);
}


void Alignment::set( Alignment::Pos h, Alignment::Pos v )
{
    hor_ = h == Start ? Left : (h == Stop ? Right : HCenter);
    ver_ = v == Start ? Top : (v == Stop ? Bottom : VCenter);
}


int Alignment::uiValue() const
{
    int ret = hor_ == Left ? 0x0001 : (hor_ == Right ? 0x0002 : 0x0004);
    ret |= ver_ == Top ? 0x0020 : (ver_ == Bottom ? 0x0040 : 0x0080);
    return ret;
}


void Alignment::setUiValue( int v )
{
    hor_ = v&0x0001 ? Left : (v&0x0002 ? Right : HCenter);
    ver_ = v&0x0020 ? Top : (v&0x0040 ? Bottom : VCenter);
}


#define mToStringImpl( clss, par ) \
void clss::toString( BufferString& bs ) const \
{ \
    FileMultiString fms; \
    fms = getTypeString(type_); \
    fms += par; \
    color_.fill( bs.buf() ); \
    fms += FileMultiString(bs); \
    bs = fms; \
}


#define mFromStringImpl( clss, par ) \
void clss::fromString( const char* s ) \
{ \
    FileMultiString fms( s ); \
    parseEnumType( fms[0], type_ ); \
    par = toInt(fms[1]); \
    FileMultiString colfms( fms.from(2) ); \
    color_.use( colfms ); \
}


mToStringImpl( MarkerStyle2D, size_ )
mToStringImpl( MarkerStyle3D, size_ )
mToStringImpl( LineStyle, width_ )

mFromStringImpl( MarkerStyle2D, size_ )
mFromStringImpl( MarkerStyle3D, size_ )
mFromStringImpl( LineStyle, width_ )

MarkerStyle2D::MarkerStyle2D(Type tp, int sz, Color col, float rot )
    : type_(tp), size_(sz), color_(col), rotation_(rot)
{}


bool MarkerStyle2D::operator==( const MarkerStyle2D& b ) const
{
    return type_==b.type_ && size_==b.size_ && color_==b.color_ &&
	mIsEqual(rotation_,b.rotation_,mDefEps);
}


const MarkerStyle2D& MarkerStyle2D::operator=( const MarkerStyle2D& a )
{
    type_ = a.type_ ; size_ = a.size_; color_ = a.color_;
    rotation_ = a.rotation_; return *this;
}


bool MarkerStyle2D::isVisible() const
{ return type_!=None && size_>0 && color_.isVisible(); }


MarkerStyle3D::MarkerStyle3D(Type tp, int sz, Color col )
    : type_(tp), size_(sz), color_(col)
{}


bool MarkerStyle3D::operator==(const MarkerStyle3D& b) const
{ return type_==b.type_ && size_==b.size_ && color_==b.color_; }


bool MarkerStyle3D::operator!=(const MarkerStyle3D& b) const
{ return !(*this==b); }


bool MarkerStyle3D::isVisible() const
{ return type_!=None && size_>0 && color_.isVisible(); }


LineStyle::LineStyle( Type t, int w, Color c )
    : type_(t), width_(w), color_(c)        
{}


bool LineStyle::operator ==( const LineStyle& ls ) const
{ return type_ == ls.type_ && width_ == ls.width_ && color_ == ls.color_; }


bool LineStyle::operator !=( const LineStyle& ls ) const
{ return !(*this == ls); }


bool LineStyle::isVisible() const
{ return type_!=None && width_>0 && color_.isVisible();}


void FillPattern::getTypeNames( BufferStringSet& res )
{
    res.add( "No Fill" );
    res.add( "Dots" );
    res.add( "Lines" );
}


void FillPattern::getOptNames( int typ, BufferStringSet& res )
{
    res.setEmpty();
    if ( typ == cDotsFillPatternType )
    {
	res.add( "Uniform color" );
	res.add( "Extremely dense" );
	res.add( "Very dense" );
	res.add( "Somewhat dense" );
	res.add( "Half dense" );
	res.add( "Somewhat sparse" );
	res.add( "Very sparse" );
	res.add( "Extremely sparse" );
    }
    else if ( typ == cLinesFillPatternType )
    {
	res.add( "Horizontal lines" );
	res.add( "Vertical lines" );
	res.add( "Crossing horizontal and vertical lines" );
	res.add( "Backward diagonal lines" );
	res.add( "Forward diagonal lines" );
	res.add( "Crossing diagonal lines" );
    }
    // else none
}



ArrowHeadStyle::ArrowHeadStyle( int sz, Type t, HandedNess h )
    : sz_(sz), type_(t), handedness_(h)
{}


void ArrowHeadStyle::setBoldNess( int b )
{ sz_ = 3*b; }



ArrowStyle::ArrowStyle( int boldness, Type t )
    : type_(t)
    , linestyle_(LineStyle::Solid,boldness)
{ setBoldNess(boldness); }


void ArrowStyle::setBoldNess( int b )
{ linestyle_.width_ = b; headstyle_.setBoldNess(b); tailstyle_.setBoldNess(b); }


bool ArrowStyle::hasHead() const
{ return headstyle_.sz_ > 0 && type_ < TailOnly; }


bool ArrowStyle::hasTail() const
{ return tailstyle_.sz_ > 0 && (type_ == TwoSided || type_ == TailOnly); }


