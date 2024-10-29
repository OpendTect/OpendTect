/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vistext.h"

#include "dirlist.h"
#include "iopar.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "separstr.h"
#include "texttranslator.h"
#include "vismaterial.h"


#include <osgGeo/Text>

mCreateFactoryEntry( visBase::Text2 );


namespace visBase
{

#define cObjectSizeToScreenSizeFactor 10 // experience const

Text::Text()
    : osgtext_( new osgGeo::Text )
{
    refOsgPtr( osgtext_ );
    osgtext_->setAxisAlignment( osgText::TextBase::SCREEN );
    osgtext_->setCharacterSizeMode(osgText::TextBase::SCREEN_COORDS );
    osgtext_->setDataVariance( osg::Object::DYNAMIC );

    //trigger update of font_
    setFontData( fontdata_, DataObject::getDefaultPixelDensity() );
}


Text::~Text()
{
    unRefOsgPtr( osgtext_ );
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
    if( scenespace )
	osgpos = Conv::to<osg::Vec3>(pos);
    else
	mVisTrans::transform( displaytrans_.ptr(), pos, osgpos );

    setPosition( osgpos );
}


Coord3 Text::getPosition() const
{
    Coord3 pos;
    Transformation::transformBack( displaytrans_.ptr(), osgtext_->getPosition(),
				   pos );
    return pos;
}


void Text::setFontData( const FontData& fd, float pixeldensity )
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


void Text::updateFontSize( float pixeldensity )
{
    const float sizefactor =
	pixeldensity / DataObject::getDefaultPixelDensity();
    osgtext_->setCharacterSize( fontdata_.pointSize() * sizefactor );
}


static const wchar_t emptystring[] =  { 0 } ;
void Text::setText( const uiString& newtext )
{
    ArrPtrMan<wchar_t> wcharbuf = newtext.createWCharString();

    if ( wcharbuf )
	osgtext_->setText( wcharbuf.ptr() );
    else
	osgtext_->setText( emptystring );

    text_ = newtext;
}


void Text::setRotation( float radangle, const Coord3& axis )
{
    osg::Quat rotation;
    rotation.makeRotate( radangle, osg::Vec3d(axis.x_, axis.y_, axis.z_) );
    osgtext_->setRotation( rotation );
}


void Text::setJustification( Justification just )
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


int Text::getJustification() const
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


void Text::setCharacterSizeMode( CharacterSizeMode mode )
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


void Text::setAxisAlignment( AxisAlignment axis )
{
    osgText::TextBase::AxisAlignment osgaxis =
	( osgText::TextBase::AxisAlignment ) axis;
    osgtext_->setAxisAlignment( osgaxis );
}



void Text::setColor( const OD::Color& col )
{
    osgtext_->setColor( Conv::to<osg::Vec4>(col) );
}


OD::Color Text::getColor() const
{
    return Conv::to<::OD::Color>( osgtext_->getColor() );
}


void Text::setDisplayTransformation( const mVisTrans* newtrans )
{
    const Coord3 oldpos = getPosition();
    displaytrans_ = newtrans;
    setPosition( oldpos );
}



Text2::Text2()
    : VisualObjectImpl(false)
    , geode_(new osg::Geode)
    , pixeldensity_(getDefaultPixelDensity())
{
    ref();
    mAttachCB( TrMgr().languageChange, Text2::translationChangeCB );
    refOsgPtr( geode_ );
    geode_->setNodeMask( ~cBBoxTraversalMask() );
    addChild( geode_ );
    geode_->setCullingActive( false );
    setPickable( false, false );
    unRefNoDelete();
}


Text2::~Text2()
{
    detachAllNotifiers();
    unRefOsgPtr( geode_ );
}


int Text2::addText()
{
    auto* newtext = new Text;
    newtext->setDisplayTransformation( displaytransform_.ptr() );
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
    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setFontData( fd, pixeldensity_ );
}


void Text2::setDisplayTransformation( const mVisTrans* newtr )
{
    displaytransform_ = newtr;
    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setDisplayTransformation( newtr );
}


void Text2::setPixelDensity( float dpi )
{
    if ( mIsEqual(pixeldensity_,dpi,0.1f) )
	return;

    pixeldensity_ = dpi;

    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->updateFontSize( pixeldensity_ );
}


void Text2::translationChangeCB(CallBacker *)
{
    for ( int idx=0; idx<texts_.size(); idx++ )
        texts_[idx]->setText( texts_[idx]->getText() );
}


PtrMan<OsgFontCreator> creator;


osgText::Font* OsgFontCreator::create( const FontData& fd )
{
    return creator ? creator->createFont(fd) : nullptr;
}


void OsgFontCreator::setCreator( OsgFontCreator* cr)
{
    creator = cr;
}


} // namespace visBase
