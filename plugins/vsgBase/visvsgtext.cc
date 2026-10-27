/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgtext.h"

#include "bufstring.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "visvsgtransform.h"
#include "math2.h"
#include "texttranslator.h"

#include <string>

mCreateDGBVisFactoryEntry( dGBVis::Text2 );

namespace dGBVis
{

// An experience value, as in visBase::Text
static constexpr float cObjectSizeToScreenSizeFactor = 10.f;


/*!Converts UTF-8 text to the UTF-32 text that vsg::Text takes. A byte
   sequence that is not valid UTF-8 gives the replacement character U+FFFD. */
static std::u32string toUtf32( const char* utf8 )
{
    static const char32_t replacement = 0xFFFD;
    static const char32_t minvalues[] = { 0, 0x80, 0x800, 0x10000 };
    std::u32string ret;
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>( utf8 );
    while ( ptr && *ptr )
    {
	const unsigned char lead = *ptr++;
	int nrextra = 0;
	char32_t code = lead;
	if ( lead >= 0xF0 && lead < 0xF8 )
	{
	    nrextra = 3;
	    code = lead & 0x07;
	}
	else if ( lead >= 0xE0 && lead < 0xF0 )
	{
	    nrextra = 2;
	    code = lead & 0x0F;
	}
	else if ( lead >= 0xC0 && lead < 0xE0 )
	{
	    nrextra = 1;
	    code = lead & 0x1F;
	}
	else if ( lead >= 0x80 )
	{
	    ret.push_back( replacement );
	    continue;
	}

	bool valid = true;
	for ( int idx=0; idx<nrextra && valid; idx++ )
	{
	    valid = (*ptr & 0xC0) == 0x80;
	    if ( valid )
		code = ( code << 6 ) | ( *ptr++ & 0x3F );
	}

	if ( code < minvalues[nrextra] || code > 0x10FFFF ||
	     (code >= 0xD800 && code <= 0xDFFF) )
	    valid = false;

	ret.push_back( valid ? code : replacement );
    }

    return ret;
}


Text::Text()
    : vsgtext_(vsgGeo::Text::create())
    , pixeldensity_(DataObject::getDefaultPixelDensity())
{
    vsgtext_->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    mAttachCB( FontCreator::instance().fontLoaded, Text::fontLoadedCB );
    setFontData( fontdata_, pixeldensity_ );
}


Text::~Text()
{
    detachAllNotifiers();
}


void Text::prepare()
{
    vsgtext_->setUpdater( DataObject::getUpdater() );
}


void Text::redraw()
{
    DataObject::requestSingleRedraw();
}


void Text::setPosition( const vsg::vec3& pos )
{
    prepare();
    vsgtext_->setPosition( pos );
    redraw();
}


void Text::setPosition( const Coord3& pos, bool scenespace )
{
    vsg::vec3 vsgpos;
    if ( scenespace )
	vsgpos.set( float(pos.x_), float(pos.y_), float(pos.z_) );
    else
	Transformation::transform( displaytrans_.ptr(), pos, vsgpos );

    setPosition( vsgpos );
}


Coord3 Text::getPosition() const
{
    Coord3 pos;
    Transformation::transformBack( displaytrans_.ptr(),
				   vsgtext_->position(), pos );
    return pos;
}


void Text::setRotation( float radangle, const Coord3& axis )
{
    prepare();
    vsgtext_->setRotation( radangle, vsg::vec3(float(axis.x_),float(axis.y_),
					       float(axis.z_)) );
    redraw();
}


void Text::setFontData( const FontData& fd, float pixeldensity )
{
    fontdata_ = fd;
    prepare();

    FontCreator& creator = FontCreator::instance();
    vsg::ref_ptr<vsg::Font> font = creator.get( fontdata_ );
    if ( font )
	vsgtext_->setFont( font );
    else
	creator.request( fontdata_ );

    updateFontSize( pixeldensity );
}


void Text::fontLoadedCB( CallBacker* )
{
    vsg::ref_ptr<vsg::Font> font = FontCreator::instance().get( fontdata_ );
    if ( !font || font==vsgtext_->font() )
	return;

    prepare();
    vsgtext_->setFont( font );
    redraw();
}


void Text::updateFontSize( float pixeldensity )
{
    pixeldensity_ = pixeldensity;
    const float sizefactor =
	pixeldensity / DataObject::getDefaultPixelDensity();
    float height = fontdata_.pointSize() * sizefactor;
    const bool screensized = sizemode_ == Screen;
    if ( !screensized )
	height *= cObjectSizeToScreenSizeFactor;

    prepare();
    vsgtext_->setScreenSized( screensized );
    vsgtext_->setCharacterHeight( height );
    redraw();
}


void Text::setText( const uiString& newtext )
{
    text_ = newtext;
    BufferString utf8;
    newtext.fillUTF8String( utf8 );
    prepare();
    vsgtext_->setText( toUtf32(utf8.buf()) );
    redraw();
}


void Text::setColor( const OD::Color& col )
{
    color_ = col;
    prepare();
    vsgtext_->setColor( toVec4(col) );
    redraw();
}


void Text::setJustification( Justification just )
{
    using HAlign = vsgGeo::Text::HAlign;
    using VAlign = vsgGeo::Text::VAlign;

    HAlign halign = HAlign::Center;
    VAlign valign = VAlign::Center;
    switch ( just )
    {
	case Left:
	    halign = HAlign::Left;
	    break;
	case Right:
	    halign = HAlign::Right;
	    break;
	case Top:
	    valign = VAlign::Top;
	    break;
	case Bottom:
	    valign = VAlign::Bottom;
	    break;
	case TopLeft:
	    halign = HAlign::Left;
	    valign = VAlign::Top;
	    break;
	case TopRight:
	    halign = HAlign::Right;
	    valign = VAlign::Top;
	    break;
	case BottomLeft:
	    halign = HAlign::Left;
	    valign = VAlign::Bottom;
	    break;
	case BottomRight:
	    halign = HAlign::Right;
	    valign = VAlign::Bottom;
	    break;
	case Center:
	    break;
    }

    justification_ = just;
    prepare();
    vsgtext_->setAlignment( halign, valign );
    redraw();
}


void Text::setDisplayTransformation( const mDGBVisTrans* newtrans )
{
    const Coord3 oldpos = getPosition();
    displaytrans_ = newtrans;
    setPosition( oldpos );
}


void Text::setCharacterSizeMode( CharacterSizeMode mode )
{
    if ( mode == sizemode_ )
	return;

    sizemode_ = mode;
    updateFontSize( pixeldensity_ );
}


void Text::setAxisAlignment( AxisAlignment axis )
{
    using AxisAlign = vsgGeo::Text::AxisAlignment;

    AxisAlign vsgaxis = AxisAlign::Screen;
    switch ( axis )
    {
	case XY:
	    vsgaxis = AxisAlign::XY;
	    break;
	case ReversedXY:
	    vsgaxis = AxisAlign::ReversedXY;
	    break;
	case XZ:
	    vsgaxis = AxisAlign::XZ;
	    break;
	case ReversedXZ:
	    vsgaxis = AxisAlign::ReversedXZ;
	    break;
	case YZ:
	    vsgaxis = AxisAlign::YZ;
	    break;
	case ReversedYZ:
	    vsgaxis = AxisAlign::ReversedYZ;
	    break;
	case User:
	    vsgaxis = AxisAlign::User;
	    break;
	case OnScreen:
	    break;
    }

    prepare();
    vsgtext_->setAxisAlignment( vsgaxis );
    redraw();
}


// Text2

Text2::Text2()
    : VisualObjectImpl(false)
    , pixeldensity_(getDefaultPixelDensity())
{
    ref();
    mAttachCB( TrMgr().languageChange, Text2::translationChangeCB );
    unRefNoDelete();
}


Text2::~Text2()
{
    detachAllNotifiers();
}


int Text2::addText()
{
    auto* newtext = new Text;
    newtext->setDisplayTransformation( displaytransform_.ptr() );
    texts_ += newtext;
    addChild( &newtext->getVsgText() );
    return texts_.size()-1;
}


void Text2::removeText( const Text* txt )
{
    const int idx = texts_.indexOf( txt );
    if ( idx<0 )
	return;

    removeChild( &texts_[idx]->getVsgText() );
    texts_.removeSingle( idx );
}


void Text2::removeAll()
{
    while ( !texts_.isEmpty() )
	removeText( texts_.last() );
}


const Text* Text2::text( int idx ) const
{
    return texts_.validIdx( idx ) ? texts_[idx] : nullptr;
}


Text* Text2::text( int idx )
{
    if ( !idx && !texts_.size() )
	addText();

    return texts_.validIdx( idx ) ? texts_[idx] : nullptr;
}


void Text2::setFontData( const FontData& fd )
{
    for ( auto* txt : texts_ )
	txt->setFontData( fd, pixeldensity_ );
}


void Text2::setDisplayTransformation( const mDGBVisTrans* newtr )
{
    displaytransform_ = newtr;
    for ( auto* txt : texts_ )
	txt->setDisplayTransformation( newtr );
}


void Text2::setPixelDensity( float dpi )
{
    if ( mIsEqual(pixeldensity_,dpi,0.1f) )
	return;

    pixeldensity_ = dpi;
    for ( auto* txt : texts_ )
	txt->updateFontSize( pixeldensity_ );
}


void Text2::translationChangeCB( CallBacker* )
{
    for ( auto* txt : texts_ )
	txt->setText( txt->getText() );
}

} // namespace dGBVis
