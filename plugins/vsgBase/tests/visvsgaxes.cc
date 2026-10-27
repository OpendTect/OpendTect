/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgaxes.h"
#include "visvsgcamera.h"
#include "visvsgdata.h"
#include "visvsgdatagroup.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "thread.h"
#include "vsggeotext.h"
#include "vsgtesthelper.h"

#include <chrono>
#include <cstdlib>
#include <thread>

namespace
{

constexpr uint32_t cSize = 256;
constexpr int cCenterX = 128;
constexpr int cCenterY = 128;


struct WhiteCount
{
    int		nrpix_ = 0;
    int		nrright_ = 0;
    int		nrbelow_ = 0;
};


WhiteCount countWhite( vsgTest::OffscreenViewer& ov )
{
    WhiteCount ret;
    TypeSet<vsg::ubvec4> pixels;
    if ( !ov.readColor(pixels) )
	return ret;

    for ( uint32_t y=0; y<cSize; y++ )
    {
	for ( uint32_t x=0; x<cSize; x++ )
	{
	    const vsg::ubvec4& pix = pixels[int(y*cSize + x)];
	    if ( pix.r<=200 || pix.g<=200 || pix.b<=200 )
		continue;

	    ret.nrpix_++;
	    if ( int(x) > cCenterX+50 )
		ret.nrright_++;

	    if ( int(y) > cCenterY+40 )
		ret.nrbelow_++;
	}
    }

    return ret;
}


struct CommonViewerGuard
{
    ~CommonViewerGuard()
    {
	dGBVis::DataObject::setCommonViewer( nullptr );
	dGBVis::FontCreator::instance().clear();
    }
};

} // namespace


static bool testAxes()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "dGBVis axes: Off-screen viewer",
			       ov.errMsg().c_str() );

    vsgGeo::Text::setViewportHeight( float(cSize) );
    CommonViewerGuard guard;
    dGBVis::DataObject::setCommonViewer( &ov.viewer() );
    dGBVis::FontCreator::instance().clear();

    RefMan<dGBVis::Camera> camera = dGBVis::Camera::create();
    camera->vsgCamera()->viewMatrix = ov.camera().viewMatrix;
    RefMan<dGBVis::DataObjectGroup> hudscene =
					    dGBVis::DataObjectGroup::create();
    ov.addHud()->addChild( vsg::ref_ptr<vsg::Node>(hudscene->vsgNode()) );

    RefMan<dGBVis::Axes> axes = dGBVis::Axes::create();
    axes->setSize( 5.f, 55.f );
    axes->setPosition( float(cCenterX), float(cSize-cCenterY) );
    axes->setPrimaryCamera( camera.ptr() );
    axes->setAnnotationFont( FontData(30,"Arial",FontData::Normal,false) );
    hudscene->addObject( axes.ptr() );
    mRunStandardTest( ov.compile(), "dGBVis axes: Viewer compiled" );

    ov.frame( 2 );
    const vsgTest::Bounds arrows = vsgTest::brightBounds( ov );
    mRunStandardTestWithError( arrows.nrpix_>100 && arrows.nrred_<arrows.nrpix_,
			       "dGBVis axes: The arrows are drawn at once",
			       BufferString("Pixels: ",arrows.nrpix_) );
    mRunStandardTest( countWhite(ov).nrpix_==0,
		      "dGBVis axes: No label is drawn before its font is "
		      "loaded" );

    WhiteCount white;
    for ( int idx=0; idx<1500 && white.nrpix_<20; idx++ )
    {
	ov.frame( 1 );
	white = countWhite( ov );
	std::this_thread::sleep_for( std::chrono::milliseconds(20) );
    }

    ov.frame( 2 );
    white = countWhite( ov );
    BufferString msg( "White pixels: ", white.nrpix_ );
    msg.add( ", right of the arrows: " ).add( white.nrright_ )
       .add( ", below them: " ).add( white.nrbelow_ );
    mRunStandardTestWithError( white.nrpix_>=20,
			       "dGBVis axes: The labels appear when the font "
			       "is loaded", msg );
    mRunStandardTestWithError( white.nrright_>5 && white.nrbelow_>5,
			       "dGBVis axes: The labels are at the tips of "
			       "the arrows", msg );
    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "dGBVis axes: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    axes = nullptr;
    hudscene = nullptr;
    ov.viewer().deviceWaitIdle();
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    if ( !testAxes() )
	return 1;

    return 0;
}
