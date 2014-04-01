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
#include "texttranslator.h"


#include <osgText/Text>

mCreateFactoryEntry( visBase::Text2 );


namespace visBase
{

Text::Text()
    : osgtext_( new osgText::Text )
    , displaytrans_( 0 )
{
    osgtext_->ref();
    osgtext_->setAxisAlignment( osgText::TextBase::SCREEN );
    osgtext_->setCharacterSizeMode(osgText::TextBase::SCREEN_COORDS );
    osgtext_->setDataVariance( osg::Object::DYNAMIC );
    setFontData( fontdata_ ); //trigger update of font_
}


Text::~Text()
{
    if ( displaytrans_ ) displaytrans_->unRef();
    osgtext_->unref();
}


osg::Drawable& Text::getDrawable()
{ return *osgtext_; }


const osg::Drawable& Text::getDrawable() const
{ return *osgtext_; }


void Text::setPosition( const osg::Vec3f& pos )
{
    osgtext_->setPosition( pos );
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
    Transformation::transformBack( displaytrans_, osgtext_->getPosition(), pos);
    return pos;
}


void Text::setFontData( const FontData& fd )
{
    fontdata_ = fd;

    osg::ref_ptr<osgText::Font> osgfont = OsgFontCreator::create( fontdata_ );
    if ( osgfont )
	osgtext_->setFont( osgfont );
    osgtext_->setCharacterSize( fontdata_.pointSize() );
}


static const wchar_t emptystring[] =  { 0 } ;
void Text::setText( const uiString& newtext )
{
    ArrPtrMan<wchar_t> wcharbuf = newtext.createWCharString();

    if ( !wcharbuf )
	osgtext_->setText( emptystring );
    else
	osgtext_->setText( wcharbuf );

    text_ = newtext;
}


void Text::setJustification( Justification just )
{
    if ( just==Center )
	osgtext_->setAlignment(osgText::TextBase::CENTER_CENTER );
    else if ( just==Left )
	osgtext_->setAlignment( osgText::TextBase::LEFT_CENTER );
    else
	osgtext_->setAlignment( osgText::TextBase::RIGHT_CENTER );
}


void Text::setColor( const Color& col )
{
    osgtext_->setColor( Conv::to<osg::Vec4>(col) );
}


Color Text::getColor() const
{
    return Conv::to<Color>( osgtext_->getColor() );
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
    mAttachCB( TrMgr().languageChange, Text2::translationChangeCB );
    geode_->ref();
    geode_->setNodeMask( ~visBase::cBBoxTraversalMask() );
    addChild( geode_ );
    mAttachOneFrameCullDisabler( geode_ );
}


Text2::~Text2()
{
    detachAllNotifiers();
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

    
void Text2::translationChangeCB(CallBacker *)
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
