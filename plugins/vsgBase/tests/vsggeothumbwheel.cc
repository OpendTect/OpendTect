/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeothumbwheel.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace
{

constexpr uint32_t cSize = 256;
constexpr double cPi = 3.14159265358979;
constexpr double cTickAngle = 20. * cPi / 180.;
constexpr double cLength = 100.;
constexpr double cWidth = 16.;
constexpr double cHorCenterX = 70.;
constexpr double cHorCenterY = 30.;
constexpr double cVerCenterX = 230.;
constexpr double cVerCenterY = 128.;

using Pixels = TypeSet<vsg::ubvec4>;


struct WheelSpec
{
    bool	horizontal_;
    double	along_;
    double	across_;
};


WheelSpec horizontalSpec()
{
    return { true, cHorCenterX, cHorCenterY };
}


WheelSpec verticalSpec()
{
    return { false, cVerCenterY, cVerCenterX };
}


vsg::ref_ptr<vsgGeo::ThumbWheel> createWheel( const WheelSpec& spec,
					      bool fade )
{
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = vsgGeo::ThumbWheel::create();
    const float alongmin = float( spec.along_ - cLength/2. );
    const float alongmax = float( spec.along_ + cLength/2. );
    const float acrossmin = float( spec.across_ - cWidth/2. );
    const float acrossmax = float( spec.across_ + cWidth/2. );
    const vsg::vec2 min = spec.horizontal_ ? vsg::vec2( alongmin, acrossmin )
					   : vsg::vec2( acrossmin, alongmin );
    const vsg::vec2 max = spec.horizontal_ ? vsg::vec2( alongmax, acrossmax )
					   : vsg::vec2( acrossmax, alongmax );
    wheel->setShape( spec.horizontal_, min, max, -1.f );
    wheel->enableFadeInOut( fade );
    return wheel;
}


int grayAt( const Pixels& pixels, const WheelSpec& spec, double along,
	    int offset )
{
    const int alongpix = int( std::floor(along) ) + offset;
    const int acrosspix = int( spec.across_ );
    const int xpix = spec.horizontal_ ? alongpix : acrosspix;
    const int ypix = spec.horizontal_ ? acrosspix : alongpix;
    if ( xpix<0 || ypix<0 || xpix>=int(cSize) || ypix>=int(cSize) )
	return 0;

    return pixels[ int((cSize-1-ypix)*cSize + xpix) ].r;
}


int maxGrayNear( const Pixels& pixels, const WheelSpec& spec, double along,
		 int reach )
{
    int ret = 0;
    for ( int offset=-reach; offset<=reach; offset++ )
	ret = std::max( ret, grayAt(pixels,spec,along,offset) );

    return ret;
}


double alongAt( const WheelSpec& spec, double angle )
{
    return spec.along_ + cLength/2.*std::cos( angle );
}


BufferString checkTicks( const Pixels& pixels, const WheelSpec& spec,
			 double phase, int maxbetween=25 )
{
    const double minangle = 45. * cPi / 180.;
    const double maxangle = 135. * cPi / 180.;
    BufferString ret;
    int nrticks = 0;
    for ( int idx=-30; idx<40; idx++ )
    {
	const double angle = (idx+1./16.)*cTickAngle - phase;
	if ( angle<minangle || angle>maxangle )
	    continue;

	nrticks++;
	const int bright = maxGrayNear( pixels, spec, alongAt(spec,angle), 1 );
	if ( bright < 60 )
	{
	    ret.set( "No tick at " ).add( angle*180./cPi );
	    return ret.add( " degrees, gray: " ).add( bright );
	}

	const double between = angle + cTickAngle/2.;
	if ( between > maxangle )
	    continue;

	const int dark = maxGrayNear( pixels, spec, alongAt(spec,between), 0 );
	if ( dark > maxbetween )
	{
	    ret.set( "A tick between the ticks at " ).add( between*180./cPi );
	    return ret.add( " degrees, gray: " ).add( dark );
	}
    }

    if ( nrticks < 4 )
	return ret.set( "Only " ).add( nrticks ).add( " ticks checked" );

    return ret.setEmpty();
}


void waitAndRender( vsgTest::OffscreenViewer& ov, int millisec )
{
    ov.frame( 1 );
    std::this_thread::sleep_for( std::chrono::milliseconds(millisec) );
    ov.frame( 2 );
}


vsg::ref_ptr<vsg::ButtonPressEvent> pressEvent( int x, int ytop,
						uint32_t button=1 )
{
    return vsg::ButtonPressEvent::create( nullptr, vsg::clock::now(), x, ytop,
					  vsg::BUTTON_MASK_1, button );
}


vsg::ref_ptr<vsg::MoveEvent> moveEvent( int x, int ytop )
{
    return vsg::MoveEvent::create( nullptr, vsg::clock::now(), x, ytop,
				   vsg::BUTTON_MASK_1 );
}


vsg::ref_ptr<vsg::ButtonReleaseEvent> releaseEvent( int x, int ytop )
{
    return vsg::ButtonReleaseEvent::create( nullptr, vsg::clock::now(), x, ytop,
					    vsg::BUTTON_MASK_OFF, 1 );
}

} // namespace


static bool testWheelDrawing()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Wheel drawing: Off-screen viewer",
			       ov.errMsg().c_str() );

    const WheelSpec horspec = horizontalSpec();
    const WheelSpec verspec = verticalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> horwheel = createWheel( horspec, false );
    vsg::ref_ptr<vsgGeo::ThumbWheel> verwheel = createWheel( verspec, false );
    vsg::ref_ptr<vsg::Group> hud = ov.addHud();
    hud->addChild( horwheel );
    hud->addChild( verwheel );
    mRunStandardTest( ov.compile(), "Wheel drawing: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Wheel drawing: Image is read" );
    BufferString msg = checkTicks( pixels, horspec, 0. );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel drawing: A horizontal wheel has a tick "
			       "every 20 degrees", msg );
    msg = checkTicks( pixels, verspec, 0. );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel drawing: A vertical wheel has a tick "
			       "every 20 degrees", msg );

    const int outside = maxGrayNear( pixels, horspec, horspec.along_ +
				     cLength/2. + 10., 3 );
    mRunStandardTestWithError( outside==0,
			       "Wheel drawing: Nothing is drawn outside the "
			       "wheel", BufferString("Gray: ",outside) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Wheel drawing: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testWheelTurns()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Wheel turns: Off-screen viewer",
			       ov.errMsg().c_str() );

    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, false );
    ov.addHud()->addChild( wheel );
    mRunStandardTest( ov.compile(), "Wheel turns: Viewer compiled" );

    const float angle = 0.5f;
    wheel->setAngle( angle );
    mRunStandardTest( wheel->getAngle()==angle,
		      "Wheel turns: The angle is kept" );
    ov.frame( 3 );
    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Wheel turns: Image is read" );
    BufferString msg = checkTicks( pixels, spec, angle );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel turns: The ticks move along with the "
			       "angle", msg );

    wheel->setAngle( -1.f );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Wheel turns: Second image is read" );
    msg = checkTicks( pixels, spec, -1. );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel turns: The ticks move back with a "
			       "negative angle", msg );

    wheel->setAngle( 0.25f, 0.1f );
    mRunStandardTest( wheel->getAngle()==0.25f,
		      "Wheel turns: An animated turn has its angle at once" );
    waitAndRender( ov, 250 );
    waitAndRender( ov, 50 );
    mRunStandardTest( ov.readColor(pixels),
		      "Wheel turns: Third image is read" );
    msg = checkTicks( pixels, spec, 0.25 );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel turns: An animated turn ends with the "
			       "ticks at the angle", msg );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Wheel turns: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testWheelFade()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Wheel fade: Off-screen viewer",
			       ov.errMsg().c_str() );

    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, true );
    wheel->setFadeTime( 0.05f );
    ov.addHud()->addChild( wheel );
    mRunStandardTest( ov.compile(), "Wheel fade: Viewer compiled" );
    mRunStandardTest( wheel->isFadeInOutEnabled(),
		      "Wheel fade: Fade in and out is on" );
    waitAndRender( ov, 100 );

    const double tickalong = alongAt( spec, 80. * cPi / 180. + cTickAngle/16. );
    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Wheel fade: Image is read" );
    mRunStandardTestWithError( wheel->getOpacity()==0.f &&
			       maxGrayNear(pixels,spec,tickalong,2)==0,
			       "Wheel fade: A wheel is not drawn when the "
			       "pointer is far away",
			       BufferString("Opacity: ",wheel->getOpacity()) );

    mRunStandardTest( !wheel->handlePointer(
		vsgGeo::ThumbWheel::Pointer::Move,
		float(spec.along_), 200.f ),
		"Wheel fade: A pointer near the wheel is not taken" );
    waitAndRender( ov, 100 );
    waitAndRender( ov, 20 );
    mRunStandardTest( ov.readColor(pixels),
		      "Wheel fade: Second image is read" );
    mRunStandardTestWithError( wheel->getOpacity()==1.f &&
			       maxGrayNear(pixels,spec,tickalong,2)>60,
			       "Wheel fade: A wheel is drawn when the pointer "
			       "comes near",
			       BufferString("Opacity: ",wheel->getOpacity()) );

    wheel->handlePointer( vsgGeo::ThumbWheel::Pointer::Move, 250.f, 250.f );
    waitAndRender( ov, 100 );
    waitAndRender( ov, 20 );
    mRunStandardTest( ov.readColor(pixels), "Wheel fade: Third image is read" );
    mRunStandardTestWithError( wheel->getOpacity()==0.f &&
			       maxGrayNear(pixels,spec,tickalong,2)==0,
			       "Wheel fade: A wheel fades out when the pointer "
			       "goes away",
			       BufferString("Opacity: ",wheel->getOpacity()) );

    wheel->setAngle( 0.1f );
    ov.frame( 1 );
    mRunStandardTestWithError( wheel->getOpacity()==1.f,
			       "Wheel fade: A wheel that turns is drawn",
			       BufferString("Opacity: ",wheel->getOpacity()) );
    waitAndRender( ov, 100 );
    waitAndRender( ov, 20 );
    mRunStandardTestWithError( wheel->getOpacity()==0.f,
			       "Wheel fade: A wheel that turned fades out "
			       "again", BufferString("Opacity: ",
						     wheel->getOpacity()) );

    wheel->enableFadeInOut( false );
    ov.frame( 3 );
    mRunStandardTestWithError( wheel->getOpacity()==1.f,
			       "Wheel fade: Without fade in and out the wheel "
			       "is always drawn",
			       BufferString("Opacity: ",wheel->getOpacity()) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Wheel fade: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testWheelDrag()
{
    const WheelSpec horspec = horizontalSpec();
    const WheelSpec verspec = verticalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> horwheel = createWheel( horspec, false );
    vsg::ref_ptr<vsgGeo::ThumbWheel> verwheel = createWheel( verspec, false );
    vsg::ref_ptr<vsgGeo::ThumbWheelHandler> handler =
					vsgGeo::ThumbWheelHandler::create();
    handler->setWindowHeight( cSize );
    handler->addThumbWheel( horwheel.get() );
    handler->addThumbWheel( verwheel.get() );
    handler->addThumbWheel( horwheel.get() );

    float horturned = 0.f;
    float verturned = 0.f;
    int nrhorcalls = 0;
    horwheel->setRotationCallback( [&]( float delta )
				   { horturned += delta; nrhorcalls++; } );
    verwheel->setRotationCallback( [&]( float delta )
				   { verturned += delta; } );

    const int horx = int( horspec.along_ );
    const int hory = int( cSize - horspec.across_ );
    vsg::ref_ptr<vsg::MoveEvent> movetest = moveEvent( horx, hory );
    movetest->accept( *handler );
    mRunStandardTest( !movetest->handled && !horwheel->isTracking(),
		      "Wheel drag: A move over a wheel is not taken" );

    vsg::ref_ptr<vsg::ButtonPressEvent> rightpress =
					    pressEvent( horx, hory, 3 );
    rightpress->accept( *handler );
    mRunStandardTest( !rightpress->handled && !horwheel->isTracking(),
		      "Wheel drag: A press with another button than the left "
		      "is not taken" );

    vsg::ref_ptr<vsg::ButtonPressEvent> misspress = pressEvent( 200, 30 );
    misspress->accept( *handler );
    mRunStandardTest( !misspress->handled && !horwheel->isTracking() &&
		      !verwheel->isTracking(),
		      "Wheel drag: A press beside the wheels is not taken" );

    vsg::ref_ptr<vsg::ButtonPressEvent> press = pressEvent( horx, hory );
    press->accept( *handler );
    mRunStandardTest( press->handled && horwheel->isTracking() &&
		      !verwheel->isTracking(),
		      "Wheel drag: A press on a wheel is taken by that wheel" );

    vsg::ref_ptr<vsg::MoveEvent> drag = moveEvent( horx+25, hory );
    drag->accept( *handler );
    mRunStandardTestWithError( drag->handled &&
			       std::abs(horwheel->getAngle()-0.5f)<1e-4f,
			       "Wheel drag: Moving the pointer along half the "
			       "radius turns the wheel half a radian",
			       BufferString("Angle: ",horwheel->getAngle()) );
    mRunStandardTestWithError( nrhorcalls==1 &&
			       std::abs(horturned+0.5f)<1e-4f,
			       "Wheel drag: The callback gets the old angle "
			       "minus the new one",
			       BufferString("Turned: ",horturned) );

    vsg::ref_ptr<vsg::MoveEvent> farmove = moveEvent( horx-10, hory-120 );
    farmove->accept( *handler );
    mRunStandardTestWithError( farmove->handled &&
			       std::abs(horwheel->getAngle()+0.2f)<1e-4f,
			       "Wheel drag: A wheel follows the pointer also "
			       "when it is off the wheel",
			       BufferString("Angle: ",horwheel->getAngle()) );

    vsg::ref_ptr<vsg::ButtonReleaseEvent> release = releaseEvent( horx, hory );
    release->accept( *handler );
    mRunStandardTest( release->handled && !horwheel->isTracking(),
		      "Wheel drag: The release ends the drag" );
    vsg::ref_ptr<vsg::MoveEvent> after = moveEvent( horx+40, hory );
    after->accept( *handler );
    mRunStandardTestWithError( !after->handled &&
			       std::abs(horwheel->getAngle()+0.2f)<1e-4f,
			       "Wheel drag: A move after the release does not "
			       "turn the wheel",
			       BufferString("Angle: ",horwheel->getAngle()) );

    const int verx = int( verspec.across_ );
    const int very = int( cSize - verspec.along_ );
    vsg::ref_ptr<vsg::ButtonPressEvent> verpress = pressEvent( verx, very );
    verpress->accept( *handler );
    vsg::ref_ptr<vsg::MoveEvent> up = moveEvent( verx, very-25 );
    up->accept( *handler );
    mRunStandardTestWithError( verpress->handled && up->handled &&
			       std::abs(verwheel->getAngle()-0.5f)<1e-4f &&
			       std::abs(verturned+0.5f)<1e-4f,
			       "Wheel drag: A vertical wheel turns with the "
			       "pointer going up",
			       BufferString("Angle: ",verwheel->getAngle()) );
    vsg::ref_ptr<vsg::ButtonReleaseEvent> verrelease =
						releaseEvent( verx, very-25 );
    verrelease->accept( *handler );
    mRunStandardTest( verrelease->handled && !verwheel->isTracking(),
		      "Wheel drag: The release ends the drag of a vertical "
		      "wheel" );

    handler->removeThumbWheel( horwheel.get() );
    vsg::ref_ptr<vsg::ButtonPressEvent> removed = pressEvent( horx, hory );
    removed->accept( *handler );
    mRunStandardTest( !removed->handled && !horwheel->isTracking(),
		      "Wheel drag: A wheel that is removed from the handler "
		      "gets no events" );
    return true;
}


static bool testWheelShape()
{
    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, false );
    mRunStandardTest( wheel->isHorizontal() && wheel->getMin().x==20.f &&
		      wheel->getMax().x==120.f,
		      "Wheel shape: The shape is kept" );

    wheel->handlePointer( vsgGeo::ThumbWheel::Pointer::Press,
			  float(spec.along_), float(spec.across_) );
    mRunStandardTest( wheel->isTracking(),
		      "Wheel shape: A press on the wheel starts a drag" );
    wheel->setShape( true, vsg::vec2(30.f,30.f), vsg::vec2(130.f,46.f), 0.f );
    mRunStandardTest( !wheel->isTracking() && wheel->getMin().x==30.f,
		      "Wheel shape: A new shape ends a drag" );
    return true;
}


static bool testWheelGrow()
{
    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, false );
    const float xpos = float( spec.along_ );
    const float besidey = float( spec.across_ + cWidth*0.75 );
    using Pointer = vsgGeo::ThumbWheel::Pointer;
    using Clock = std::chrono::steady_clock;
    const Clock::time_point start = Clock::now();
    wheel->advance( start );
    wheel->handlePointer( Pointer::Move, xpos, besidey );
    wheel->advance( start + std::chrono::seconds(1) );
    mRunStandardTestWithError( !wheel->isGrowOnHover() &&
			       wheel->getGrowth()==0.f,
			       "Wheel grow: A wheel does not grow unless asked",
			       BufferString("Growth: ",wheel->getGrowth()) );
    mRunStandardTest( !wheel->handlePointer(Pointer::Press,xpos,besidey),
		      "Wheel grow: A press beside the thin wheel is not "
		      "taken" );

    wheel->setGrowOnHover( true );
    wheel->advance( start + std::chrono::seconds(2) );
    mRunStandardTestWithError( wheel->getGrowth()==1.f,
			       "Wheel grow: A wheel with the pointer near it "
			       "grows",
			       BufferString("Growth: ",wheel->getGrowth()) );
    mRunStandardTest( wheel->handlePointer(Pointer::Press,xpos,besidey) &&
		      wheel->isTracking(),
		      "Wheel grow: The grown wheel is caught where the thin "
		      "one was missed" );
    wheel->handlePointer( Pointer::Release, xpos, besidey );

    wheel->handlePointer( Pointer::Move, 250.f, 250.f );
    wheel->advance( start + std::chrono::seconds(3) );
    mRunStandardTestWithError( wheel->getGrowth()==0.f,
			       "Wheel grow: A wheel gets thin again when the "
			       "pointer goes away",
			       BufferString("Growth: ",wheel->getGrowth()) );
    mRunStandardTest( !wheel->handlePointer(Pointer::Press,xpos,besidey) &&
		      !wheel->isTracking(),
		      "Wheel grow: The wheel that got thin again is not caught "
		      "beside the line" );
    return true;
}


static bool testWheelGrowArea()
{
    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, true );
    wheel->setGrowOnHover( true );
    const float xpos = float( spec.along_ );
    const float nearbyy = float( spec.across_ + cWidth*1.25 );
    const float insidey = float( spec.across_ + cWidth*0.75 );
    using Pointer = vsgGeo::ThumbWheel::Pointer;
    using Clock = std::chrono::steady_clock;
    const Clock::time_point start = Clock::now();
    wheel->advance( start );
    wheel->handlePointer( Pointer::Move, xpos, nearbyy );
    wheel->advance( start + std::chrono::seconds(1) );
    wheel->advance( start + std::chrono::seconds(3) );
    mRunStandardTestWithError( wheel->getGrowth()==0.f &&
			       wheel->getOpacity()>0.9f,
			       "Wheel grow: A wheel that the pointer is near "
			       "fades in but does not grow until the pointer "
			       "is in the area it will cover",
			       BufferString("Growth: ",wheel->getGrowth())
				.add(", opacity: ").add(wheel->getOpacity()) );

    wheel->handlePointer( Pointer::Move, xpos, insidey );
    wheel->advance( start + std::chrono::seconds(4) );
    mRunStandardTestWithError( wheel->getGrowth()==1.f,
			       "Wheel grow: A wheel grows when the pointer "
			       "gets to the edge of the area it will cover",
			       BufferString("Growth: ",wheel->getGrowth()) );

    wheel->handlePointer( Pointer::Move, xpos, nearbyy );
    wheel->advance( start + std::chrono::seconds(5) );
    mRunStandardTestWithError( wheel->getGrowth()==0.f,
			       "Wheel grow: A wheel gets thin again when the "
			       "pointer leaves the area it covers",
			       BufferString("Growth: ",wheel->getGrowth()) );
    return true;
}


static bool testWheelGlass()
{
    vsgTest::OffscreenViewer ov( cSize, cSize );
    mRunStandardTestWithError( ov.isOK(), "Wheel glass: Off-screen viewer",
			       ov.errMsg().c_str() );

    const WheelSpec spec = horizontalSpec();
    vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, false );
    wheel->setStyle( vsgGeo::ThumbWheel::Style::Glass );
    mRunStandardTest( wheel->style()==vsgGeo::ThumbWheel::Style::Glass,
		      "Wheel glass: The style is kept" );
    ov.addHud()->addChild( wheel );
    mRunStandardTest( ov.compile(), "Wheel glass: Viewer compiled" );
    ov.frame( 3 );

    Pixels pixels;
    mRunStandardTest( ov.readColor(pixels), "Wheel glass: Image is read" );
    const BufferString msg = checkTicks( pixels, spec, 0., 80 );
    mRunStandardTestWithError( msg.isEmpty(),
			       "Wheel glass: The glass wheel has a tick every "
			       "20 degrees", msg );

    const double between = 91.25 * cPi / 180.;
    const int body = maxGrayNear( pixels, spec, alongAt(spec,between), 0 );
    mRunStandardTestWithError( body>=15 && body<=80,
			       "Wheel glass: The glass wheel is lit between "
			       "the ticks, which the classic wheel is not",
			       BufferString("Gray: ",body) );

    wheel->setStyle( vsgGeo::ThumbWheel::Style::Classic );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels),
		      "Wheel glass: Second image is read" );
    const int classicbody = maxGrayNear( pixels, spec, alongAt(spec,between),
					 0 );
    mRunStandardTestWithError( classicbody<=5,
			       "Wheel glass: The classic wheel is dark between "
			       "the ticks",
			       BufferString("Gray: ",classicbody) );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Wheel glass: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


static bool testWheelGrowLength()
{
    const WheelSpec spec = horizontalSpec();
    const float centery = float( spec.across_ );
    const float beyondmax = float( spec.along_ + cLength/2. + 15. );
    const float beyondmin = float( spec.along_ - cLength/2. - 15. );
    using Pointer = vsgGeo::ThumbWheel::Pointer;
    using Clock = std::chrono::steady_clock;
    for ( int dir=-1; dir<=1; dir++ )
    {
	vsg::ref_ptr<vsgGeo::ThumbWheel> wheel = createWheel( spec, false );
	wheel->setGrowDirection( dir );
	wheel->setGrowOnHover( true );
	const Clock::time_point start = Clock::now();
	wheel->advance( start );
	wheel->handlePointer( Pointer::Move, float(spec.along_), centery );
	wheel->advance( start + std::chrono::seconds(1) );
	const bool atmax = wheel->handlePointer( Pointer::Press, beyondmax,
						 centery );
	wheel->handlePointer( Pointer::Release, beyondmax, centery );
	const bool atmin = wheel->handlePointer( Pointer::Press, beyondmin,
						 centery );
	wheel->handlePointer( Pointer::Release, beyondmin, centery );
	mRunStandardTestWithError( atmax==(dir>=0) && atmin==(dir<=0),
				   "Wheel grow: A grown wheel is longer in the "
				   "direction it grows",
				   BufferString("Direction: ",dir)
				    .add(", max end: ").add(int(atmax))
				    .add(", min end: ").add(int(atmin)) );
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    vsgTest::installAssertTrace();

    if ( !testWheelDrawing() ||
	 !testWheelTurns() ||
	 !testWheelFade() ||
	 !testWheelDrag() ||
	 !testWheelShape() ||
	 !testWheelGrow() ||
	 !testWheelGrowArea() ||
	 !testWheelGrowLength() ||
	 !testWheelGlass() )
	return 1;

    return 0;
}
