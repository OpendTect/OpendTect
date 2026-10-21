/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgdata.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "visvsgtext.h"
#include "visvsgtransform.h"
#include "math2.h"
#include "thread.h"
#include "vsggeoupdater.h"
#include "vsgtesthelper.h"


namespace
{

class TextSetup
{
public:
    TextSetup( vsgTest::OffscreenViewer& ov )
	: ov_(ov)
    {
	dGBVis::DataObject::setCommonViewer( &ov.viewer() );
	dGBVis::FontCreator::instance().clear();
	texts_ = dGBVis::Text2::create();
	ov.scene().addChild( vsg::ref_ptr<vsg::Node>(texts_->vsgNode()) );
    }

    ~TextSetup()
    {
	texts_ = nullptr;
	ov_.scene().children.clear();
	ov_.viewer().deviceWaitIdle();
	dGBVis::DataObject::setCommonViewer( nullptr );
	dGBVis::FontCreator::instance().clear();
    }

    static FontData testFont()
    { return FontData( 5, "Arial", FontData::Normal, false ); }

    static float testDensity()
    {
	return dGBVis::DataObject::
		getDefaultPixelDensity() * 0.01f;
    }

    dGBVis::Text* addLabel( const char* str )
    {
	const int idx = texts_->addText();
	dGBVis::Text* txt = texts_->text( idx );
	txt->setCharacterSizeMode(
			dGBVis::Text::Object );
	txt->setAxisAlignment( dGBVis::Text::XZ );
	txt->setJustification( dGBVis::Text::Center );
	txt->setFontData( testFont(), testDensity() );
	txt->setText( toUiString(str) );
	return txt;
    }

    bool waitDrawn( int mincount=50 )
    {
	for ( int idx=0; idx<1500; idx++ )
	{
	    ov_.frame( 1 );
	    if ( vsgTest::brightBounds(ov_).nrpix_
					>=mincount )
		return true;

	    Threads::sleep( 0.02 );
	}

	return false;
    }

    vsgTest::Bounds bounds( int nrframes=2 )
    {
	ov_.frame( nrframes );
	return vsgTest::brightBounds( ov_ );
    }

    vsgTest::OffscreenViewer&	ov_;
    RefMan<dGBVis::Text2>	texts_;
};

} // namespace


static bool testLabelDrawn()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Label drawing: Off-screen viewer",
			       ov.errMsg().c_str() );

    TextSetup setup( ov );
    mRunStandardTest( ov.compile(), "Label drawing: Viewer compiled" );

    dGBVis::Text* txt = setup.addLabel( "Hi" );
    mRunStandardTest( setup.texts_->nrTexts()==1,
		      "Label drawing: One label added" );
    mRunStandardTest( setup.bounds().nrpix_==0,
		      "Label drawing: Nothing is drawn before the font "
		      "is loaded" );
    mRunStandardTest( setup.waitDrawn(),
		      "Label drawing: Label appears when the font is loaded" );

    const vsgTest::Bounds bnds = setup.bounds();
    BufferString msg( "Centre x: ", bnds.cx() );
    msg.add( " y: " ).add( bnds.cy() );
    mRunStandardTestWithError( abs(bnds.cx()-128)<=12 && abs(bnds.cy()-128)<=12,
			       "Label drawing: Centre-justified label is "
			       "at the anchor",
			       msg );
    mRunStandardTest( txt->getText().getFullString().isEqual("Hi"),
		      "Label drawing: The text is kept" );

    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Label drawing: No validation messages", msg );
    return true;
}


static bool testConversions()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
			       "OD type conversion: Off-screen viewer",
			       ov.errMsg().c_str() );

    TextSetup setup( ov );
    mRunStandardTest( ov.compile(), "OD type conversion: Viewer compiled" );
    dGBVis::Text* txt = setup.addLabel( "Hi" );
    mRunStandardTest( setup.waitDrawn(), "OD type conversion: Label is drawn" );
    const vsgTest::Bounds white = setup.bounds();

    txt->setColor( OD::Color(255,0,0) );
    vsgTest::Bounds bnds = setup.bounds();
    mRunStandardTest( bnds.nrred_ > bnds.nrpix_/2,
		      "OD type conversion: OD colour is used" );

    txt->setColor( OD::Color(255,255,255,255) );
    bnds = setup.bounds();
    mRunStandardTest( bnds.nrpix_==0,
		      "OD type conversion: OD transparency is used" );
    txt->setColor( OD::Color::White() );

    using Just = dGBVis::Text;
    txt->setJustification( Just::Left );
    bnds = setup.bounds();
    BufferString msg( "Left edge x: ", bnds.minx_ );
    mRunStandardTestWithError( bnds.nrpix_>0 && bnds.minx_>=124,
			       "OD type conversion: Left puts the label "
			       "right of the anchor",
			       msg );

    txt->setJustification( Just::Right );
    bnds = setup.bounds();
    msg.set( "Right edge x: " ).add( bnds.maxx_ );
    mRunStandardTestWithError( bnds.nrpix_>0 && bnds.maxx_<=132,
			       "OD type conversion: Right puts the label "
			       "left of the anchor",
			       msg );

    txt->setJustification( Just::TopRight );
    const vsgTest::Bounds topright = setup.bounds();
    txt->setJustification( Just::BottomRight );
    const vsgTest::Bounds bottomright = setup.bounds();
    msg.set( "Right edges: " ).add( topright.maxx_ ).add( " and " )
	.add( bottomright.maxx_ ).add( ", centre y top: " )
	.add( topright.cy() ).add( " bottom: " ).add( bottomright.cy() );
    mRunStandardTestWithError( topright.maxx_<=132 && bottomright.maxx_<=132 &&
			       topright.cy()>bottomright.cy(),
			       "OD type conversion: Corner justifications "
			       "combine both axes",
			       msg );
    mRunStandardTest( txt->getJustification()==Just::BottomRight,
		      "OD type conversion: Justification is kept" );

    txt->setJustification( Just::Center );
    txt->setPosition( Coord3(0.5,0.,0.) );
    bnds = setup.bounds();
    mRunStandardTestWithError( bnds.cx() > white.cx()+40,
			       "OD type conversion: Position moves the label",
			       BufferString("Centre x: ",bnds.cx()) );

    RefMan<dGBVis::Transformation> trans = dGBVis::Transformation::create();
    trans->setTranslation( Coord3(-0.5,0.,0.) );
    txt->setDisplayTransformation( trans.ptr() );
    bnds = setup.bounds();
    msg.set( "Centre x: " ).add( bnds.cx() );
    mRunStandardTestWithError( abs(bnds.cx()-white.cx())<=4,
			       "OD type conversion: Display transformation "
			       "is applied", msg );
    const Coord3 backpos = txt->getPosition();
    mRunStandardTest( mIsEqual(backpos.x_,0.5,1e-4) &&
		      mIsZero(backpos.y_,1e-4) && mIsZero(backpos.z_,1e-4),
		      "OD type conversion: Position is given back "
		      "in survey coordinates" );

    txt->setText( toUiString("H") );
    bnds = setup.bounds();
    mRunStandardTest( bnds.nrpix_>0 && bnds.nrpix_<white.nrpix_,
		      "OD type conversion: Shorter text draws fewer pixels" );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    txt->setPosition( Coord3(0.,0.,0.) );
    txt->setDisplayTransformation( nullptr );
    txt->setJustification( Just::Center );
    txt->setAxisAlignment( dGBVis::Text::OnScreen );
    txt->setCharacterSizeMode( dGBVis::Text::Screen );
    txt->setFontData( TextSetup::testFont(),
		      dGBVis::DataObject::getDefaultPixelDensity() * 8.f );
    ov.frame( 4 );
    bnds = setup.bounds( 3 );
    const int screenheight = bnds.maxy_ - bnds.miny_ + 1;
    msg.set( "Height of an H in pixels: " ).add( screenheight );
    mRunStandardTestWithError( bnds.nrpix_>0 && screenheight>=20 &&
			       screenheight<=40,
			       "OD type conversion: Screen size mode gives "
			       "a size in pixels (40 requested)", msg );

    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "OD type conversion: No validation messages",
			       msg );
    return true;
}


static bool testText2()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Text2: Off-screen viewer",
			       ov.errMsg().c_str() );

    TextSetup setup( ov );
    mRunStandardTest( ov.compile(), "Text2: Viewer compiled" );
    dGBVis::Text* left = setup.addLabel( "Hi" );
    dGBVis::Text* right = setup.addLabel( "Hi" );
    left->setPosition( Coord3(-0.5,0.,0.) );
    right->setPosition( Coord3(0.5,0.,0.) );
    mRunStandardTest( setup.texts_->nrTexts()==2, "Text2: Two labels added" );
    mRunStandardTest( setup.waitDrawn(), "Text2: Labels are drawn" );

    vsgTest::Bounds bnds = setup.bounds();
    BufferString msg( "Extent x: ", bnds.minx_ );
    msg.add( " to " ).add( bnds.maxx_ );
    mRunStandardTestWithError( bnds.minx_<64 && bnds.maxx_>192,
			       "Text2: Both labels are drawn", msg );

    setup.texts_->removeText( right );
    bnds = setup.bounds( 6 );
    msg.set( "Extent x: " ).add( bnds.minx_ ).add( " to " ).add( bnds.maxx_ );
    mRunStandardTestWithError( setup.texts_->nrTexts()==1 && bnds.nrpix_>0 &&
			       bnds.maxx_<128,
			       "Text2: A removed label disappears", msg );

    setup.texts_->removeAll();
    bnds = setup.bounds( 6 );
    mRunStandardTest( setup.texts_->nrTexts()==0 && bnds.nrpix_==0,
		      "Text2: Removing all labels leaves nothing drawn" );
    mRunStandardTest( setup.texts_->text(1)==nullptr,
		      "Text2: Unknown label index gives no label" );

    mRunStandardTest( ov.nrValidationMessages()==0,
		      "Text2: No validation messages" );
    return true;
}


static bool testNonAsciiText()
{
    RefMan<dGBVis::Text2> texts = dGBVis::Text2::create();
    dGBVis::Text* txt = texts->text( texts->addText() );
    txt->setText( toUiString("A\xC3\xB8\xE2\x82\xAC\xF0\x9F\x98\x80") );
    mRunStandardTest( txt->getVsgText().text()==U"A\u00F8\u20AC\U0001F600",
		      "Text with 2, 3 and 4 byte characters is converted "
		      "to UTF-32" );

    txt->setText( toUiString("A\xFF" "B\xC3" "C") );
    mRunStandardTest( txt->getVsgText().text()==U"A\uFFFDB\uFFFDC",
		      "Bytes that are not valid UTF-8 give the replacement "
		      "character" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    if ( !testLabelDrawn() ||
	 !testConversions() ||
	 !testText2() ||
	 !testNonAsciiText() )
	return 1;

    return 0;
}
