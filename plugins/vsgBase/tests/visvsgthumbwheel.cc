/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The dGBVis thumbwheel as visBase::ThumbWheel is used. No GPU.
-*/

#include "testprog.h"

#include "visvsgdata.h"
#include "visvsgdataman.h"
#include "visvsgthumbwheel.h"
#include "thread.h"
#include "vsggeothumbwheel.h"

#include <cmath>

namespace
{

class Recorder : public CallBacker
{
public:
    void onRotation( CallBacker* cb )
    {
	mCBCapsuleUnpack(float,delta,cb);
	total_ += delta;
	count_++;
    }

    float	total_ = 0.f;
    int		count_ = 0;
};

} // namespace


static bool testPosition()
{
    RefMan<dGBVis::ThumbWheel> horwheel = dGBVis::ThumbWheel::create();
    horwheel->setPosition( true, 90.f, 22.5f, 100.f, 15.f, -1.f );
    const vsgGeo::ThumbWheel* hor = horwheel->vsgWheel();
    mRunStandardTest( hor->isHorizontal() && hor->getMin().x==40.f &&
		      hor->getMax().x==140.f && hor->getMin().y==15.f &&
		      hor->getMax().y==30.f,
		      "dGBVis thumbwheel: A horizontal wheel is centered at "
		      "the position and as long as asked in x" );

    RefMan<dGBVis::ThumbWheel> verwheel = dGBVis::ThumbWheel::create();
    verwheel->setPosition( false, 22.5f, 90.f, 100.f, 15.f, -1.f );
    const vsgGeo::ThumbWheel* ver = verwheel->vsgWheel();
    mRunStandardTest( !ver->isHorizontal() && ver->getMin().x==15.f &&
		      ver->getMax().x==30.f && ver->getMin().y==40.f &&
		      ver->getMax().y==140.f,
		      "dGBVis thumbwheel: A vertical wheel is centered at the "
		      "position and as long as asked in y" );
    return true;
}


static bool testAngleAndFade()
{
    RefMan<dGBVis::ThumbWheel> wheel = dGBVis::ThumbWheel::create();
    mRunStandardTest( wheel->isFadeInOutEnabled(),
		      "dGBVis thumbwheel: Fade in and out is on at first" );
    wheel->enableFadeInOut( false );
    mRunStandardTest( !wheel->isFadeInOutEnabled(),
		      "dGBVis thumbwheel: Fade in and out can be switched "
		      "off" );

    wheel->setAngle( 0.75f );
    mRunStandardTest( wheel->getAngle()==0.75f,
		      "dGBVis thumbwheel: The angle is kept" );
    return true;
}


static bool testRotation()
{
    RefMan<dGBVis::ThumbWheel> wheel = dGBVis::ThumbWheel::create();
    wheel->setPosition( true, 100.f, 50.f, 100.f, 16.f );
    Recorder recorder;
    wheel->rotation.notify( mCB(&recorder,Recorder,onRotation) );

    wheel->setAngle( 0.25f );
    mRunStandardTest( recorder.count_==0,
		      "dGBVis thumbwheel: Setting the angle is not a rotation "
		      "of the user" );

    vsgGeo::ThumbWheel* vsgwheel = wheel->vsgWheel();
    vsgwheel->handlePointer( vsgGeo::ThumbWheel::Pointer::Press, 100.f, 50.f );
    vsgwheel->handlePointer( vsgGeo::ThumbWheel::Pointer::Move, 125.f, 50.f );
    vsgwheel->handlePointer( vsgGeo::ThumbWheel::Pointer::Release, 125.f,
			     50.f );
    mRunStandardTestWithError( recorder.count_==1 &&
			       std::abs(recorder.total_+0.5f)<1e-4f &&
			       std::abs(wheel->getAngle()-0.75f)<1e-4f,
			       "dGBVis thumbwheel: A drag of the user is "
			       "passed as the angle before minus the angle "
			       "after",
			       BufferString("Passed: ",recorder.total_) );

    wheel->rotation.remove( mCB(&recorder,Recorder,onRotation) );
    vsg::ref_ptr<vsgGeo::ThumbWheel> kept( vsgwheel );
    wheel = nullptr;
    kept->handlePointer( vsgGeo::ThumbWheel::Pointer::Press, 100.f, 50.f );
    kept->handlePointer( vsgGeo::ThumbWheel::Pointer::Move, 120.f, 50.f );
    mRunStandardTest( recorder.count_==1,
		      "dGBVis thumbwheel: A wheel that outlives the object "
		      "does not call it" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();
    if ( !testPosition() ||
	 !testAngleAndFade() ||
	 !testRotation() )
	return 1;

    return 0;
}
