/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeotext.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include <climits>
#include <cstdlib>
#include <filesystem>

namespace
{

vsg::ref_ptr<vsg::Font> loadFont()
{
    const std::filesystem::path dir =
	    std::filesystem::path( __FILE__ ).parent_path().parent_path();
    return vsg::read_cast<vsg::Font>(
		(dir / "folder_vsg" / "times.vsgb").string() );
}


using vsgTest::Bounds;
using vsgTest::brightBounds;


vsg::ref_ptr<vsgGeo::Text> addLabel( vsgTest::OffscreenViewer& ov,
				     vsgGeo::Updater& updater,
				     vsg::ref_ptr<vsg::Font> font,
				     const std::u32string& str )
{
    vsg::ref_ptr<vsgGeo::Text> text = vsgGeo::Text::create();
    text->setUpdater( &updater );
    text->setFont( font );
    text->setText( str );
    text->setCharacterHeight( 0.5f );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::XZ );
    text->setAlignment( vsgGeo::Text::HAlign::Center,
			vsgGeo::Text::VAlign::Center );
    ov.scene().addChild( text );
    return text;
}

} // namespace


static bool testRenders()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Text rendering: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Text rendering: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"Hi" );
    mRunStandardTest( ov.compile(), "Text rendering: Viewer compiled" );
    ov.frame( 3 );

    mRunStandardTest( text->textNode()!=nullptr,
		      "Text rendering: Text node built" );
    mRunStandardTest( text->capacity()==vsgGeo::Text::sMinCapacity,
		      "Text rendering: First allocation has headroom" );

    const Bounds bnds = brightBounds( ov );
    BufferString msg( "Bright pixels: ", bnds.nrpix_ );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Text rendering: Label is drawn", msg );

    msg.set( "Centre x: " ).add( bnds.cx() ).add( " y: " ).add( bnds.cy() );
    mRunStandardTestWithError( abs(bnds.cx()-128)<=12 && abs(bnds.cy()-128)<=12,
			       "Text rendering: Centre-aligned label is at "
			       "the anchor", msg );

    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Text rendering: No failed compiles" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Text rendering: No validation messages", msg );
    return true;
}


static bool testInPlace()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "In-place update: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "In-place update: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"Hi" );
    mRunStandardTest( ov.compile(), "In-place update: Viewer compiled" );
    ov.frame( 3 );

    const vsg::Text* node = text->textNode();
    const Bounds before = brightBounds( ov );
    mRunStandardTest( node && before.nrpix_>50,
		      "In-place update: Label is drawn" );

    text->setText( U"H" );
    text->setColor( vsg::vec4(1.f,0.f,0.f,1.f) );
    ov.frame( 2 );
    const Bounds changed = brightBounds( ov );
    mRunStandardTest( text->textNode()==node,
		      "In-place update: Text and colour change keep the node" );
    mRunStandardTest( changed.nrpix_>0 && changed.nrpix_<before.nrpix_,
		      "In-place update: Shorter text draws fewer pixels" );
    mRunStandardTest( changed.nrred_ > changed.nrpix_/2,
		      "In-place update: Colour changed to red" );

    text->setPosition( vsg::vec3(0.5f,0.f,0.f) );
    ov.frame( 2 );
    const Bounds moved = brightBounds( ov );
    mRunStandardTest( text->textNode()==node,
		      "In-place update: Position change keeps the node" );
    BufferString msg( "Centre x before: ", changed.cx() );
    msg.add( " after: " ).add( moved.cx() );
    mRunStandardTestWithError( moved.cx() > changed.cx()+40,
			       "In-place update: Label moved right", msg );

    text->setText( std::u32string(vsgGeo::Text::sMinCapacity-1,U'i') );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()==node,
		      "In-place update: Text just below the allocation "
		      "keeps the node" );

    text->setText( std::u32string(vsgGeo::Text::sMinCapacity,U'i') );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()!=node,
		      "In-place update: Text filling the allocation "
		      "rebuilds the node" );
    mRunStandardTest( text->capacity()>vsgGeo::Text::sMinCapacity,
		      "In-place update: Rebuild allocates more room" );

    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "In-place update: No failed compiles" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "In-place update: No validation messages", msg );
    return true;
}


static bool testRebuild()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Text rebuild: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Text rebuild: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"Hi" );
    mRunStandardTest( ov.compile(), "Text rebuild: Viewer compiled" );
    ov.frame( 3 );

    const vsg::Text* node = text->textNode();
    mRunStandardTest( node!=nullptr, "Text rebuild: Text node built" );

    text->setCharacterHeight( 0.1f );
    text->setText( std::u32string(40,U'H') );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()!=node,
		      "Text rebuild: Text longer than the allocation "
		      "rebuilds" );
    mRunStandardTest( text->capacity()>40,
		      "Text rebuild: New allocation fits the text" );
    Bounds bnds = brightBounds( ov );
    BufferString msg( "Bright pixels: ", bnds.nrpix_ );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Text rebuild: Long label is drawn", msg );

    node = text->textNode();
    text->setFont( loadFont() );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()!=node,
		      "Text rebuild: Font change rebuilds" );
    bnds = brightBounds( ov );
    msg.set( "Bright pixels: " ).add( bnds.nrpix_ );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Text rebuild: Label is drawn with the new "
			       "font", msg );

    node = text->textNode();
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()!=node,
		      "Text rebuild: Switching billboarding on rebuilds" );
    bnds = brightBounds( ov );
    msg.set( "Bright pixels: " ).add( bnds.nrpix_ );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Text rebuild: Billboard label is drawn", msg );

    node = text->textNode();
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::XZ );
    ov.frame( 2 );
    mRunStandardTest( text->textNode()!=node,
		      "Text rebuild: Switching billboarding off rebuilds" );
    bnds = brightBounds( ov );
    msg.set( "Bright pixels: " ).add( bnds.nrpix_ );
    mRunStandardTestWithError( bnds.nrpix_>50,
			       "Text rebuild: Label is drawn again", msg );

    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Text rebuild: No failed compiles" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Text rebuild: No validation messages", msg );
    return true;
}


static bool testAlignment()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Text alignment: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Text alignment: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"Hi" );
    mRunStandardTest( ov.compile(), "Text alignment: Viewer compiled" );
    ov.frame( 3 );
    const vsg::Text* node = text->textNode();

    using HAlign = vsgGeo::Text::HAlign;
    using VAlign = vsgGeo::Text::VAlign;

    text->setAlignment( HAlign::Left, VAlign::Center );
    ov.frame( 2 );
    Bounds bnds = brightBounds( ov );
    BufferString msg( "Left edge x: ", bnds.minx_ );
    mRunStandardTestWithError( bnds.nrpix_>0 && bnds.minx_>=124,
			       "Text alignment: Left alignment puts the "
			       "label right of the anchor", msg );

    text->setAlignment( HAlign::Right, VAlign::Center );
    ov.frame( 2 );
    bnds = brightBounds( ov );
    msg.set( "Right edge x: " ).add( bnds.maxx_ );
    mRunStandardTestWithError( bnds.nrpix_>0 && bnds.maxx_<=132,
			       "Text alignment: Right alignment puts the "
			       "label left of the anchor", msg );

    text->setAlignment( HAlign::Center, VAlign::Top );
    ov.frame( 2 );
    const Bounds top = brightBounds( ov );
    text->setAlignment( HAlign::Center, VAlign::Bottom );
    ov.frame( 2 );
    const Bounds bottom = brightBounds( ov );
    msg.set( "Centre y top-aligned: " ).add( top.cy() );
    msg.add( " bottom-aligned: " ).add( bottom.cy() );
    mRunStandardTestWithError( top.nrpix_>0 && bottom.nrpix_>0 &&
			       top.cy()>bottom.cy(),
			       "Text alignment: Top alignment puts the "
			       "label below the anchor", msg );

    mRunStandardTest( text->textNode()==node,
		      "Text alignment: Alignment changes keep the node" );
    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Text alignment: No validation messages", msg );
    return true;
}


static int labelPixelHeight( vsgTest::OffscreenViewer& ov, vsgGeo::Text& text,
			     double distance, float autoscale )
{
    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,-distance,0.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,0.,1.) );
    text.setAutoScaleDistance( autoscale );
    ov.frame( 3 );
    const vsgTest::Bounds bnds = vsgTest::brightBounds( ov );
    return bnds.nrpix_>0 ? bnds.maxy_ - bnds.miny_ + 1 : 0;
}


static bool testAutoScale()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Auto-scale distance: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
			       "Auto-scale distance: Off-screen viewer",
			       ov.errMsg().c_str() );

    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"H" );
    text->setCharacterHeight( 2.f );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    mRunStandardTest( ov.compile(), "Auto-scale distance: Viewer compiled" );

    const int plain10 = labelPixelHeight( ov, *text, 10., 0.f );
    const int plain20 = labelPixelHeight( ov, *text, 20., 0.f );
    BufferString msg( "Height in pixels at distance 10: ", plain10 );
    msg.add( " and 20: " ).add( plain20 );
    mRunStandardTestWithError( plain10>0 && abs(plain10-2*plain20)<=3,
			       "Auto-scale distance: A billboard label "
			       "shrinks with distance", msg );

    const int beyond = labelPixelHeight( ov, *text, 20., 10.f );
    msg.set( "Height at distance 20 with auto-scale distance 10: " )
       .add( beyond ).add( ", without: " ).add( plain20 );
    mRunStandardTestWithError( abs(beyond-plain20)<=2,
			       "Auto-scale distance: Auto-scale does not "
			       "change a label that is farther away than "
			       "its distance", msg );

    const int closer = labelPixelHeight( ov, *text, 10., 40.f );
    msg.set( "Height at distance 10 with auto-scale distance 40: " )
       .add( closer ).add( ", without: " ).add( plain10 );
    mRunStandardTestWithError( abs(closer-plain10/4)<=3,
			       "Auto-scale distance: Auto-scale shrinks a "
			       "label that is closer than its distance, in "
			       "proportion", msg );
    return true;
}


static bool testScreenSize()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Screen-sized text: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
			       "Screen-sized text: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"H" );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    text->setScreenSized( true );
    text->setCharacterHeight( 40.f );
    mRunStandardTest( ov.compile(), "Screen-sized text: Viewer compiled" );

    const int near10 = labelPixelHeight( ov, *text, 10., 0.f );
    const int mid20 = labelPixelHeight( ov, *text, 20., 0.f );
    const int far40 = labelPixelHeight( ov, *text, 40., 0.f );
    BufferString msg( "Heights in pixels at distance 10, 20 and 40: ", near10 );
    msg.add( ", " ).add( mid20 ).add( ", " ).add( far40 );
    mRunStandardTestWithError( near10>10 && abs(near10-mid20)<=3 &&
			       abs(near10-far40)<=3,
			       "Screen-sized text: A screen-sized label "
			       "keeps its pixel height at any distance", msg );

    text->setCharacterHeight( 80.f );
    const int doubled = labelPixelHeight( ov, *text, 20., 0.f );
    msg.set( "Height for 40 and 80 pixels: " ).add( mid20 ).add( ", " )
       .add( doubled );
    mRunStandardTestWithError( abs(doubled-2*mid20)<=4,
			       "Screen-sized text: The height follows the "
			       "requested pixels", msg );

    text->setCharacterHeight( 40.f );
    ov.camera().projectionMatrix = vsg::Orthographic::create(
		-1.0, 1.0, -1.0, 1.0, 0.1, 100.0 );
    const int ortho = labelPixelHeight( ov, *text, 10., 0.f );
    msg.set( "Height in pixels, perspective: " ).add( mid20 )
       .add( ", orthographic: " ).add( ortho );
    mRunStandardTestWithError( abs(ortho-mid20)<=3,
			       "Screen-sized text: Same pixel height with an "
			       "orthographic camera", msg );

    text->setScreenSized( false );
    text->setCharacterHeight( 0.5f );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    const int objnear = labelPixelHeight( ov, *text, 10., 0.f );
    const int objfar = labelPixelHeight( ov, *text, 20., 0.f );
    msg.set( "Heights at distance 10 and 20: " ).add( objnear ).add( ", " )
       .add( objfar );
    mRunStandardTestWithError( objnear>0 && abs(objnear-2*objfar)<=3,
			       "Screen-sized text: Screen size off: the "
			       "label is object-sized again", msg );

    msg.set( "Messages: " ).add( ov.nrValidationMessages() );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Screen-sized text: No validation messages",
			       msg );
    return true;
}


// A second view of the scene with another size, as in the screenshot
static bool testSecondViewImpl( bool builtbeforecompile, bool screensized,
				const char* name, bool viewalwayson=false,
				bool viewonatcompile=false )
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    BufferString title( name );
    mRunStandardTest( font.valid(), "Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsg::Camera> othercamera;
    vsg::ref_ptr<vsg::Switch> otherview =
				ov.addSwitchedView( 512, 384, othercamera );
    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );

    vsg::ref_ptr<vsgGeo::Text> text = vsgGeo::Text::create();
    if ( !builtbeforecompile )
	text->setUpdater( updater.get() );

    text->setFont( font );
    text->setText( U"H" );
    text->setCharacterHeight( screensized ? 40.f : 1.f );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    text->setScreenSized( screensized );
    ov.scene().addChild( text );
    if ( viewalwayson || viewonatcompile )
	otherview->setAllChildren( true );

    mRunStandardTest( ov.compile(), "Viewer compiled" );
    if ( viewonatcompile )
	otherview->setAllChildren( false );

    if ( builtbeforecompile )
	text->setUpdater( updater.get() );

    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,-10.,0.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,0.,1.) );
    ov.frame( 3 );

    const vsgTest::Bounds before = vsgTest::brightBounds( ov );
    mRunStandardTestWithError( before.nrpix_>20, "Label is drawn", title );

    for ( int idx=0; idx<3; idx++ )
    {
	otherview->setAllChildren( true );
	ov.frame( 1 );
	if ( !viewalwayson )
	    otherview->setAllChildren( false );

	ov.frame( 2 );
    }

    const vsgTest::Bounds after = vsgTest::brightBounds( ov );
    BufferString msg( title );
    msg.add( ": pixels before: " ).add( before.nrpix_ ).add( ", after: " )
       .add( after.nrpix_ );
    mRunStandardTestWithError( after.nrpix_>20 &&
			       abs(after.nrpix_-before.nrpix_)<=
						    before.nrpix_/5+5,
			       "The label is the same after the second view "
			       "was drawn", msg );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "No failed compiles" );
    return true;
}


static bool testSecondView()
{
    if ( !testSecondViewImpl(true,false,
			     "Second view: prebuilt, object-sized") ||
	 !testSecondViewImpl(true,true,
			     "Second view: prebuilt, screen-sized") ||
	 !testSecondViewImpl(false,true,
			     "Second view: run time, view always on",true) ||
	 !testSecondViewImpl(false,true,
			     "Second view: run time, view on at compile",
			     false,true) )
	return false;

    return true;
}


static bool glyphShape( const TypeSet<vsg::ubvec4>& pixels, uint32_t width,
			uint32_t height, double& ratio )
{
    int minx = INT_MAX, maxx = -1, miny = INT_MAX, maxy = -1;
    for ( uint32_t y=0; y<height; y++ )
    {
	for ( uint32_t x=0; x<width; x++ )
	{
	    const vsg::ubvec4& pix = pixels[int(y*width + x)];
	    if ( pix.r<=128 && pix.g<=128 && pix.b<=128 )
		continue;

	    minx = mMIN( minx, int(x) );
	    maxx = mMAX( maxx, int(x) );
	    miny = mMIN( miny, int(y) );
	    maxy = mMAX( maxy, int(y) );
	}
    }

    if ( maxx<minx || maxy<miny )
	return false;

    ratio = double(maxx-minx+1) / double(maxy-miny+1);
    return true;
}


// A view of 4:1 with the projection of the main view (1:1), as in a squeezed
// screenshot: the main view shows the glyph shape that is expected
static bool testSqueezedView()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Squeezed view: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Squeezed view: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    vsgGeo::Text::setHorizontalScale( 1.f );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsg::Camera> othercamera;
    vsg::ref_ptr<vsg::Image> otherimage;
    vsg::ref_ptr<vsg::Switch> otherview =
		ov.addSwitchedView( 512, 128, othercamera, &otherimage );
    otherview->setAllChildren( true );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"H" );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    text->setScreenSized( true );
    text->setCharacterHeight( 80.f );
    mRunStandardTest( ov.compile(), "Squeezed view: Viewer compiled" );
    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,-10.,0.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,0.,1.) );

    TypeSet<vsg::ubvec4> pixels;
    ov.frame( 6 );
    double stretched = 0., corrected = 0., normal = 0.;
    mRunStandardTest( ov.readImage(otherimage,512,128,pixels) &&
		      glyphShape(pixels,512,128,stretched),
		      "Squeezed view: Glyph is drawn in the wide view" );

    TypeSet<vsg::ubvec4> mainpixels;
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),normal),
		      "Squeezed view: Glyph is drawn in the main view" );

    vsgGeo::Text::setHorizontalScale( 1.f/4.f );
    ov.frame( 6 );
    mRunStandardTest( ov.readImage(otherimage,512,128,pixels) &&
		      glyphShape(pixels,512,128,corrected),
		      "Squeezed view: Glyph is drawn in the wide view, "
		      "corrected" );
    vsgGeo::Text::setHorizontalScale( 1.f );

    BufferString msg( "Width/height of the glyph, wide view: ", stretched );
    msg.add( " without and " ).add( corrected ).add( " with the correction; " )
       .add( "main view: " ).add( normal );
    mRunStandardTestWithError( stretched>2.0 && corrected>0.4 &&
			       corrected<1.2 && abs(corrected-normal)<0.25,
			       "Squeezed view: The correction un-stretches "
			       "the glyph in a squeezed view", msg );
    return true;
}


/* The frames of takeScreenshot(). A: the factor is set. B: the labels are
   stretched, the main view is left out when frozen. C: the factor is set back.
   D: the labels are normal again and the main view is drawn. */
static bool testFrozenWindow( bool freeze )
{
    const BufferString title( freeze ? "Frozen window: window frozen"
				     : "Frozen window: window not frozen" );
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    vsgGeo::Text::setHorizontalScale( 1.f );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsg::Camera> othercamera;
    vsg::ref_ptr<vsg::Image> otherimage;
    vsg::ref_ptr<vsg::Switch> otherview =
		ov.addSwitchedView( 512, 128, othercamera, &otherimage, true );
    otherview->setAllChildren( true );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"H" );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    text->setScreenSized( true );
    text->setCharacterHeight( 80.f );
    mRunStandardTest( ov.compile(), "Viewer compiled" );
    otherview->setAllChildren( false );
    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,-10.,0.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,0.,1.) );

    ov.frame( 6 );
    TypeSet<vsg::ubvec4> mainpixels;
    double normal = 0.;
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),normal),
		      "Glyph is drawn in the main view" );

    vsgGeo::Text::setHorizontalScale( 1.f/4.f );
    otherview->setAllChildren( true );
    TypeSet<vsg::ubvec4> otherpixels;
    double mainA = 0., mainB = 0., mainC = 0., mainD = 0.;
    double otherA = 0., otherB = 0., otherC = 0.;
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),mainA) &&
		      ov.readImage(otherimage,512,128,otherpixels) &&
		      glyphShape(otherpixels,512,128,otherA),
		      "Glyphs are drawn in the first frame" );

    if ( freeze )
	ov.mainSwitch().setAllChildren( false );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),mainB) &&
		      ov.readImage(otherimage,512,128,otherpixels) &&
		      glyphShape(otherpixels,512,128,otherB),
		      "Glyphs are drawn in the second frame" );

    vsgGeo::Text::setHorizontalScale( 1.f );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),mainC) &&
		      ov.readImage(otherimage,512,128,otherpixels) &&
		      glyphShape(otherpixels,512,128,otherC),
		      "Glyphs are drawn in the third frame" );

    ov.mainSwitch().setAllChildren( true );
    otherview->setAllChildren( false );
    ov.frame( 1 );
    mRunStandardTest( ov.readColor(mainpixels) &&
		      glyphShape(mainpixels,ov.width(),ov.height(),mainD),
		      "Glyph is drawn in the fourth frame" );

    BufferString msg( title );
    msg.add( ": width/height of the glyph, normal: " ).add( normal )
       .add( "; main view in the 4 frames: " ).add( mainA ).add( ", " )
       .add( mainB ).add( ", " ).add( mainC ).add( ", " ).add( mainD )
       .add( "; wide view in the first 3: " ).add( otherA ).add( ", " )
       .add( otherB ).add( ", " ).add( otherC );
    mRunStandardTestWithError( otherA>2.0 && abs(otherB-normal)<0.25 &&
			       abs(otherC-normal)<0.25,
			       "The wide view is stretched in the first frame "
			       "and corrected in the next two", msg );
    mRunStandardTestWithError( abs(mainA-normal)<0.25 &&
			       abs(mainD-normal)<0.25,
			       "The main view is normal before and after", msg );
    if ( freeze )
    {
	mRunStandardTestWithError( abs(mainB-normal)<0.25 &&
				   abs(mainC-normal)<0.25,
				   "The frozen main view is never squeezed",
				   msg );
    }
    else
    {
	mRunStandardTestWithError( mainB<normal*0.6 && mainC<normal*0.6,
				   "The main view that is not frozen is "
				   "squeezed in the second and third frame",
				   msg );
    }

    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "No failed compiles" );
    return true;
}


// The same frames as above, submitted without waiting for the GPU
static bool testFrozenBackToBack()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    vsgGeo::Text::setHorizontalScale( 1.f );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1.0, 0.1, 1000.0 );
    vsg::ref_ptr<vsg::Camera> othercamera;
    vsg::ref_ptr<vsg::Image> otherimage;
    vsg::ref_ptr<vsg::Switch> otherview =
		ov.addSwitchedView( 512, 128, othercamera, &otherimage, true );
    otherview->setAllChildren( true );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> text = addLabel( ov, *updater, font, U"H" );
    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
    text->setScreenSized( true );
    text->setCharacterHeight( 80.f );
    mRunStandardTest( ov.compile(), "Viewer compiled" );
    otherview->setAllChildren( false );
    ov.camera().viewMatrix = vsg::LookAt::create(
		vsg::dvec3(0.,-10.,0.), vsg::dvec3(0.,0.,0.),
		vsg::dvec3(0.,0.,1.) );

    ov.frame( 6 );
    TypeSet<vsg::ubvec4> pixels;
    double normal = 0., mainafter = 0., otherafter = 0.;
    mRunStandardTest( ov.readColor(pixels) &&
		      glyphShape(pixels,ov.width(),ov.height(),normal),
		      "Glyph is drawn in the main view" );

    for ( int idx=0; idx<5; idx++ )
    {
	vsgGeo::Text::setHorizontalScale( 1.f/4.f );
	otherview->setAllChildren( true );
	ov.frame( 1, false );
	ov.mainSwitch().setAllChildren( false );
	ov.frame( 1, false );
	vsgGeo::Text::setHorizontalScale( 1.f );
	ov.frame( 1, false );
	ov.mainSwitch().setAllChildren( true );
	otherview->setAllChildren( false );
	ov.frame( 1, false );
	ov.frame( 1, false );
    }

    mRunStandardTest( ov.readColor(pixels) &&
		      glyphShape(pixels,ov.width(),ov.height(),mainafter) &&
		      ov.readImage(otherimage,512,128,pixels) &&
		      glyphShape(pixels,512,128,otherafter),
		      "Glyphs are drawn after the back-to-back frames" );

    BufferString msg( "Width/height of the glyph, normal: ", normal );
    msg.add( "; main view after: " ).add( mainafter )
       .add( "; wide view after: " ).add( otherafter );
    mRunStandardTestWithError( abs(mainafter-normal)<0.25 &&
			       abs(otherafter-normal)<0.25,
			       "Back-to-back frames: the main view is normal "
			       "and the wide view is corrected", msg );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "No failed compiles" );
    return true;
}


static int litWidth( const TypeSet<vsg::ubvec4>& pixels, uint32_t width,
		     uint32_t height, uint32_t x0, uint32_t x1 )
{
    int minx = INT_MAX, maxx = -1;
    for ( uint32_t y=0; y<height; y++ )
    {
	for ( uint32_t x=x0; x<x1; x++ )
	{
	    const vsg::ubvec4& pix = pixels[int(y*width + x)];
	    if ( pix.r<=128 && pix.g<=128 && pix.b<=128 )
		continue;

	    minx = mMIN( minx, int(x) );
	    maxx = mMAX( maxx, int(x) );
	}
    }

    return maxx<minx ? 0 : maxx-minx+1;
}


static bool testSharedShaderSet()
{
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Shared shader set: Font loaded" );

    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
			       "Shared shader set: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    vsg::ref_ptr<vsgGeo::Text> narrow = addLabel( ov, *updater, font, U"I" );
    vsg::ref_ptr<vsgGeo::Text> wide = addLabel( ov, *updater, font, U"WWWW" );
    narrow->setCharacterHeight( 0.25f );
    narrow->setPosition( vsg::vec3(-0.5f,0.f,0.f) );
    wide->setCharacterHeight( 0.25f );
    wide->setPosition( vsg::vec3(0.5f,0.f,0.f) );
    mRunStandardTest( ov.compile(), "Shared shader set: Viewer compiled" );
    ov.frame( 3 );

    mRunStandardTest( narrow->textNode() && wide->textNode() &&
		      narrow->textNode()->shaderSet==wide->textNode()->shaderSet,
		      "Shared shader set: The labels of a viewer share "
		      "the shader set" );

    TypeSet<vsg::ubvec4> pixels;
    mRunStandardTest( ov.readColor(pixels),
		      "Shared shader set: Image is read" );
    const int leftwidth = litWidth( pixels, ov.width(), ov.height(), 0,
				    ov.width()/2 );
    const int rightwidth = litWidth( pixels, ov.width(), ov.height(),
				     ov.width()/2, ov.width() );
    BufferString msg( "Width of the narrow label: ", leftwidth );
    msg.add( ", of the wide label: " ).add( rightwidth );
    mRunStandardTestWithError( leftwidth>0 && rightwidth>3*leftwidth,
			       "Shared shader set: Labels that share the "
			       "shader set show their own text", msg );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "Shared shader set: No failed compiles" );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Shared shader set: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    vsgTest::installAssertTrace();

    if ( !testRenders() ||
	 !testInPlace() ||
	 !testRebuild() ||
	 !testAlignment() ||
	 !testAutoScale() ||
	 !testScreenSize() ||
	 !testSecondView() ||
	 !testSqueezedView() ||
	 !testFrozenWindow(false) ||
	 !testFrozenWindow(true) ||
	 !testFrozenBackToBack() ||
	 !testSharedShaderSet() )
	return 1;

    return 0;
}
