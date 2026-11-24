/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "timefun.h"
#include "vsggeotext.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include <cmath>
#include <filesystem>
#include <functional>
#include <vector>

namespace
{

constexpr int sNrLabels = 1000;
constexpr int sNrColumns = 40;
constexpr int sNrFrames = 30;
constexpr double sMaxFrameMs = 100.;
constexpr double sMaxFirstFrameMs = 5000.;

vsg::ref_ptr<vsg::Font> loadFont()
{
    const std::filesystem::path dir =
	    std::filesystem::path( __FILE__ ).parent_path().parent_path();
    return vsg::read_cast<vsg::Font>(
		(dir / "folder_vsg" / "times.vsgb").string() );
}


std::u32string labelText( int idx, int shift )
{
    const std::string str = "Well " + std::to_string( idx + shift );
    return std::u32string( str.begin(), str.end() );
}


vsg::ref_ptr<vsg::LookAt> lookFrom( double distance )
{
    return vsg::LookAt::create( vsg::dvec3(0.,-distance,0.),
				vsg::dvec3(0.,0.,0.), vsg::dvec3(0.,0.,1.) );
}


double averageFrameMs( vsgTest::OffscreenViewer& ov, int nrframes,
		       const std::function<void(int)>& prepare )
{
    double total = 0.;
    Time::Counter counter;
    for ( int idx=0; idx<nrframes; idx++ )
    {
	prepare( idx );
	counter.start();
	ov.frame( 1 );
	total += counter.elapsed();
    }

    return total / nrframes;
}

} // namespace


static bool testManyLabels( bool screensized )
{
    const BufferString title( screensized
				? "Many labels: screen-sized labels"
				: "Many labels: object-sized labels" );
    vsg::ref_ptr<vsg::Font> font = loadFont();
    mRunStandardTest( font.valid(), "Font loaded" );

    vsgTest::OffscreenViewer ov( 1024, 768 );
    mRunStandardTestWithError( ov.isOK(), "Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(ov.height()) );
    ov.camera().projectionMatrix = vsg::Perspective::create(
		30.0, 1024.0/768.0, 0.1, 1000.0 );
    ov.camera().viewMatrix = lookFrom( 10. );

    vsg::ref_ptr<vsgGeo::Updater> updater =
				vsgGeo::Updater::install( ov.viewer() );
    std::vector<vsg::ref_ptr<vsgGeo::Text>> texts;
    Time::Counter counter;
    counter.start();
    for ( int idx=0; idx<sNrLabels; idx++ )
    {
	vsg::ref_ptr<vsgGeo::Text> text = vsgGeo::Text::create();
	text->setUpdater( updater.get() );
	text->setFont( font );
	text->setText( labelText(idx,0) );
	text->setPosition( vsg::vec3( -2.4f + 0.12f*(idx%sNrColumns), 0.f,
				      -1.5f + 0.12f*(idx/sNrColumns) ) );
	text->setAlignment( vsgGeo::Text::HAlign::Center,
			    vsgGeo::Text::VAlign::Center );
	if ( screensized )
	{
	    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::Screen );
	    text->setScreenSized( true );
	    text->setCharacterHeight( 12.f );
	}
	else
	{
	    text->setAxisAlignment( vsgGeo::Text::AxisAlignment::XZ );
	    text->setCharacterHeight( 0.08f );
	}

	ov.scene().addChild( text );
	texts.push_back( text );
    }

    const int createms = counter.elapsed();
    mRunStandardTest( ov.compile(), "Viewer compiled" );

    counter.start();
    ov.frame( 1 );
    const int firstms = counter.elapsed();
    ov.frame( 3 );

    const vsgTest::Bounds drawn = vsgTest::brightBounds( ov );
    mRunStandardTestWithError( drawn.nrpix_>5000, "The labels are drawn",
			       BufferString("Pixels: ",drawn.nrpix_) );

    const double staticms = averageFrameMs( ov, sNrFrames, [](int){} );
    const double movingms = averageFrameMs( ov, sNrFrames, [&ov](int idx)
	{
	    ov.camera().viewMatrix = lookFrom( 10. + 0.5*std::sin(0.3*idx) );
	} );
    ov.camera().viewMatrix = lookFrom( 10. );
    const double changingms = averageFrameMs( ov, sNrFrames, [&texts](int idx)
	{
	    for ( int lidx=0; lidx<sNrLabels; lidx++ )
		texts[lidx]->setText( labelText(lidx,idx+1) );
	} );

    od_cout() << title << ": create " << int(createms) << " ms, first frame "
	      << int(firstms) << " ms; per frame: static " << int(staticms)
	      << " ms, moving camera " << int(movingms) << " ms, changing text "
	      << int(changingms) << " ms" << od_endl;

    BufferString msg( title );
    msg.add( ": static " ).add( staticms ).add( ", moving " ).add( movingms )
       .add( ", changing " ).add( changingms ).add( " ms per frame" );
    mRunStandardTestWithError(
	staticms<sMaxFrameMs && movingms<sMaxFrameMs && changingms<sMaxFrameMs,
	"1000 labels keep an interactive frame rate", msg );
    mRunStandardTestWithError( firstms<sMaxFirstFrameMs,
			       "1000 labels appear within a few seconds",
			       BufferString("First frame: ",firstms) );
    mRunStandardTest( updater->compiler().nrFailedCompiles()==0,
		      "No failed compiles" );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    vsgTest::installAssertTrace();

    if ( !testManyLabels(false) ||
	 !testManyLabels(true) )
	return 1;

    return 0;
}
