/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/

#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "visosg.h"
#include "separstr.h"
#include "dirlist.h"
#include "oddirs.h"
#include "texttranslation.h"


#include <osgGeo/Text>

mCreateFactoryEntry( visBase::Text );


namespace visBase
{

#define cObjectSizeToScreenSizeFactor 10 // experience const

TextDrawable::TextDrawable()
    : osgtext_( new osgGeo::Text )
    , displaytrans_( 0 )
{
    osgtext_->ref();
    osgtext_->setAxisAlignment( osgText::TextBase::SCREEN );
    osgtext_->setCharacterSizeMode(osgText::TextBase::SCREEN_COORDS );
    osgtext_->setDataVariance( osg::Object::DYNAMIC );

    //trigger update of font_
    setFontData( fontdata_, DataObject::getDefaultPixelDensity() );
}


TextDrawable::~TextDrawable()
{
    if ( displaytrans_ ) displaytrans_->unRef();
    osgtext_->unref();
}


osg::Drawable& TextDrawable::getDrawable()
{ return *osgtext_; }


const osg::Drawable& TextDrawable::getDrawable() const
{ return *osgtext_; }


void TextDrawable::setPosition( const osg::Vec3f& pos )
{
    osgtext_->setPosition( pos );
}


void TextDrawable::setPosition( const Coord3& pos, bool scenespace )
{
    osg::Vec3 osgpos;
    if( !scenespace )
	mVisTrans::transform( displaytrans_, pos, osgpos );
    else
	osgpos = Conv::to<osg::Vec3>(pos);

    setPosition( osgpos );
}


Coord3 TextDrawable::getPosition() const
{
    Coord3 pos;
    Transformation::transformBack( displaytrans_, osgtext_->getPosition(), pos);
    return pos;
}


void TextDrawable::setFontData( const FontData& fd, float pixeldensity )
{
    fontdata_ = fd;
    if ( osgtext_->getCharacterSizeMode() == osgText::TextBase::OBJECT_COORDS )
    {
	fontdata_.setPointSize(
	    fontdata_.pointSize()*cObjectSizeToScreenSizeFactor );
    }
    osg::ref_ptr<osgText::Font> osgfont = OsgFontCreator::create( fontdata_ );
    if ( osgfont )
	osgtext_->setFont( osgfont );

    updateFontSize( pixeldensity );
}


void TextDrawable::updateFontSize( float pixeldensity )
{
    const float sizefactor =
	pixeldensity / DataObject::getDefaultPixelDensity();
    osgtext_->setCharacterSize( fontdata_.pointSize() * sizefactor );
}


static const wchar_t emptystring[] =  { 0 } ;
void TextDrawable::setText( const uiString& newtext )
{
    ArrPtrMan<wchar_t> wcharbuf = newtext.createWCharString();

    osgtext_->setText( mFromUiStringTodo(newtext) );
    if ( !wcharbuf )
	osgtext_->setText( emptystring );
    else
	osgtext_->setText( wcharbuf );

    text_ = newtext;
}


void TextDrawable::setJustification( Justification just )
{
    if ( just == Center )
	osgtext_->setAlignment(osgText::TextBase::CENTER_CENTER );
    else if ( just == Left )
	osgtext_->setAlignment( osgText::TextBase::LEFT_CENTER );
    else if ( just == Right )
	osgtext_->setAlignment( osgText::TextBase::RIGHT_CENTER );
    else if ( just == Top )
	 osgtext_->setAlignment( osgText::TextBase::CENTER_TOP );
    else if ( just == Bottom )
	osgtext_->setAlignment( osgText::TextBase::CENTER_BOTTOM );
    else if ( just == TopLeft )
	 osgtext_->setAlignment( osgText::TextBase::LEFT_TOP );
    else if ( just == TopRight )
	 osgtext_->setAlignment( osgText::TextBase::RIGHT_TOP );
    else if ( just == BottomLeft )
	 osgtext_->setAlignment( osgText::TextBase::LEFT_BOTTOM );
    else if ( just == BottomRight )
	 osgtext_->setAlignment( osgText::TextBase::RIGHT_BOTTOM );
}


int TextDrawable::getJustification() const
{
    if ( osgtext_->getAlignment()==osgText::TextBase::LEFT_CENTER )
	return Left;
    if ( osgtext_->getAlignment()==osgText::TextBase::RIGHT_CENTER )
	return Right;
    if ( osgtext_->getAlignment()==osgText::TextBase::CENTER_TOP )
	return Top;
    if ( osgtext_->getAlignment()==osgText::TextBase::CENTER_BOTTOM )
	return Bottom;
    if ( osgtext_->getAlignment()==osgText::TextBase::LEFT_TOP )
	return TopLeft;
    if ( osgtext_->getAlignment()==osgText::TextBase::RIGHT_TOP )
	return TopRight;
    if ( osgtext_->getAlignment()==osgText::TextBase::LEFT_BOTTOM )
	return BottomLeft;
    if ( osgtext_->getAlignment()==osgText::TextBase::RIGHT_BOTTOM )
	return BottomRight;

    return Center;
}


void TextDrawable::setCharacterSizeMode( CharacterSizeMode mode )
{
    const osgText::TextBase::CharacterSizeMode osgmode =
	( osgText::TextBase::CharacterSizeMode ) mode;

    const osgText::TextBase::CharacterSizeMode oldosgmode =
	osgtext_->getCharacterSizeMode();

    osgtext_->setCharacterSizeMode( osgmode );

    if ( osgmode == oldosgmode )
	return;

    if ( osgmode == osgText::TextBase::OBJECT_COORDS &&
	oldosgmode == osgText::TextBase::SCREEN_COORDS )
    {
	osgtext_->setCharacterSize(
	    osgtext_->getCharacterHeight()*cObjectSizeToScreenSizeFactor );
    }
    else if( osgmode == osgText::TextBase::SCREEN_COORDS &&
	oldosgmode == osgText::TextBase::OBJECT_COORDS )
    {
	osgtext_->setCharacterSize(
	    osgtext_->getCharacterHeight()/cObjectSizeToScreenSizeFactor);
    }

}


void TextDrawable::setAxisAlignment( AxisAlignment axis )
{
    osgText::TextBase::AxisAlignment osgaxis =
	( osgText::TextBase::AxisAlignment ) axis;
    osgtext_->setAxisAlignment( osgaxis );
}



void TextDrawable::setColor( const Color& col )
{
    osgtext_->setColor( Conv::to<osg::Vec4>(col) );
}


Color TextDrawable::getColor() const
{
    return Conv::to<Color>( osgtext_->getColor() );
}


void TextDrawable::setDisplayTransformation( const mVisTrans* newtrans )
{
    const Coord3 oldpos = getPosition();

    if ( displaytrans_ ) displaytrans_->unRef();
    displaytrans_ = newtrans;
    if ( displaytrans_ ) displaytrans_->ref();

    setPosition( oldpos );
}


Text::Text()
    : VisualObjectImpl( false )
    , geode_( new osg::Geode )
    , displaytransform_( 0 )
    , pixeldensity_( getDefaultPixelDensity() )
{
    mAttachCB( TrMgr().languageChange, Text::translationChangeCB );
    geode_->ref();
    geode_->setNodeMask( ~visBase::cBBoxTraversalMask() );
    addChild( geode_ );
    geode_->setCullingActive( false );
    setPickable( false, false );
}


Text::~Text()
{
    detachAllNotifiers();
    if ( displaytransform_ ) displaytransform_->unRef();
    geode_->unref();
}


int Text::addText()
{
    TextDrawable* newtext = new TextDrawable;
    newtext->setDisplayTransformation( displaytransform_ );
    texts_ += newtext;
    geode_->addDrawable( &newtext->getDrawable() );
    return texts_.size()-1;
}


void Text::removeText( const TextDrawable* txt )
{
    const int idx = texts_.indexOf( txt );
    if ( idx<0 )
	return;

    geode_->removeDrawable( &texts_[idx]->getDrawable() );
    texts_.removeSingle( idx );
}


void Text::removeAll()
{
    geode_->removeDrawables( 0, geode_->getNumDrawables() );
    texts_.erase();
}


const TextDrawable* Text::text( int idx ) const
{
    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}


TextDrawable* Text::text( int idx )
{
    if ( !idx && !texts_.size() )
	addText();

    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}


void Text::setFontData( const FontData& fd )
{
    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setFontData( fd, pixeldensity_ );
}


void Text::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = newtr;
    if ( displaytransform_ ) displaytransform_->ref();

    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setDisplayTransformation( newtr );
}


void Text::setPixelDensity( float dpi )
{
    if ( pixeldensity_==dpi )
	return;

    pixeldensity_ = dpi;

    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->updateFontSize( pixeldensity_ );
}


void Text::translationChangeCB(CallBacker *)
{
    for ( int idx=0; idx<texts_.size(); idx++ )
        texts_[idx]->setText( texts_[idx]->getText() );
}


PtrMan<OsgFontCreator> creator = 0;


osgText::Font* OsgFontCreator::create(const FontData& fd)
{
    return creator ? creator->createFont(fd) : 0;
}


void OsgFontCreator::setCreator( OsgFontCreator* cr)
{
    creator = cr;
}


}; // namespace visBase
