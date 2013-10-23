/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "visosg.h"
#include "separstr.h"
#include "keystrs.h"
#include "dirlist.h"
#include "oddirs.h"
#include "filepath.h"


#include <osgText/Text>

mCreateFactoryEntry( visBase::Text2 );


namespace visBase
{

Text::Text()
    : text_( new osgText::Text )
    , displaytrans_( 0 )
{
    text_->ref();
    text_->setAxisAlignment( osgText::TextBase::SCREEN );
    text_->setCharacterSizeMode(osgText::TextBase::SCREEN_COORDS );
    text_->setDataVariance( osg::Object::DYNAMIC );
    setFontData( fontdata_ ); //trigger update of font_
}


Text::~Text()
{
    if ( displaytrans_ ) displaytrans_->unRef();
    text_->unref();
}


osg::Drawable& Text::getDrawable()
{ return *text_; }


const osg::Drawable& Text::getDrawable() const
{ return *text_; }


void Text::setPosition( const osg::Vec3f& pos )
{
    text_->setPosition( pos );
}


void Text::setPosition( const Coord3& pos, bool scenespace )
{
    osg::Vec3 osgpos;
    if( !scenespace )
	mVisTrans::transform( displaytrans_, pos, osgpos );
    else
	osgpos = Conv::to<osg::Vec3>(pos);

    setPosition( osgpos );
}


Coord3 Text::getPosition() const
{
    Coord3 pos;
    Transformation::transformBack( displaytrans_, text_->getPosition(), pos );
    return pos;
}


void Text::setFontData( const FontData& fd )
{
    fontdata_ = fd;

    osg::ref_ptr<osgText::Font> osgfont = OsgFontCreator::create( fontdata_ );
    if ( osgfont )
	text_->setFont( osgfont );
    text_->setCharacterSize( fontdata_.pointSize() );
}



void Text::setText( const char* newtext )
{
    text_->setText( newtext );
}


void Text::getText( BufferString& res ) const
{
    std::string str = text_->getText().createUTF8EncodedString();
    res = str.data();
}


void Text::setJustification( Justification just )
{
    if ( just==Center )
	text_->setAlignment(osgText::TextBase::CENTER_CENTER );
    else if ( just==Left )
	text_->setAlignment( osgText::TextBase::LEFT_CENTER );
    else
	text_->setAlignment( osgText::TextBase::RIGHT_CENTER );
}


void Text::setColor( const Color& col )
{
    text_->setColor( Conv::to<osg::Vec4>(col) );
}


Color Text::getColor() const
{
    return Conv::to<Color>( text_->getColor() );
}


void Text::setDisplayTransformation( const mVisTrans* newtrans )
{
    const Coord3 oldpos = getPosition();

    if ( displaytrans_ ) displaytrans_->unRef();
    displaytrans_ = newtrans;
    if ( displaytrans_ ) displaytrans_->ref();

    setPosition( oldpos );
}


Text2::Text2()
    : VisualObjectImpl( false )
    , geode_( new osg::Geode )
    , displaytransform_( 0 )
{
    geode_->ref();
    geode_->setNodeMask( ~visBase::cBBoxTraversalMask() );
    addChild( geode_ );
    mAttachOneFrameCullDisabler( geode_ );
}


Text2::~Text2()
{
    if ( displaytransform_ ) displaytransform_->unRef();
    geode_->unref();
}


int Text2::addText()
{
    Text* newtext = new Text;
    newtext->setDisplayTransformation( displaytransform_ );
    texts_ += newtext;
    geode_->addDrawable( &newtext->getDrawable() );

    return texts_.size()-1;
}


void Text2::removeText( const Text* txt )
{
    const int idx = texts_.indexOf( txt );
    if ( idx<0 )
	return;

    geode_->removeDrawable( &texts_[idx]->getDrawable() );
    texts_.removeSingle( idx );
}


void Text2::removeAll()
{
    geode_->removeDrawables( 0, geode_->getNumDrawables() );
    texts_.erase();
}


const Text* Text2::text( int idx ) const
{
    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}


Text* Text2::text( int idx )
{
    if ( !idx && !texts_.size() )
	addText();

    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}


void Text2::setFontData( const FontData& fd )
{
    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setFontData( fd );
}


void Text2::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = newtr;
    if ( displaytransform_ ) displaytransform_->ref();

    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setDisplayTransformation( newtr );
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
