/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The trackball moves the camera as the thumbwheels do, and reports the
 movements the user makes. No GPU.
-*/

#include "testprog.h"

#include "vsggeotrackball.h"

#include <vsg/app/Camera.h>
#include <vsg/app/ViewMatrix.h>
#include <vsg/maths/mat4.h>
#include <vsg/state/ViewportState.h>

#include <cmath>

namespace
{

constexpr double cPi = 3.14159265358979;
constexpr double cDistance = 10.;

struct Moves
{
    int		nrcalls_ = 0;
    float	hor_ = 0.f;
    float	vert_ = 0.f;
    float	dist_ = 0.f;
    bool	step_ = false;
};


vsg::ref_ptr<vsg::Camera> createCamera()
{
    return vsg::Camera::create(
		vsg::Perspective::create( 30., 1., 0.1, 1000. ),
		vsg::LookAt::create( vsg::dvec3(0.,-cDistance,0.),
				     vsg::dvec3(0.,0.,0.),
				     vsg::dvec3(0.,0.,1.) ),
		vsg::ViewportState::create( VkExtent2D{256,256} ) );
}


void resetCamera( vsg::LookAt& lookat )
{
    lookat.eye = vsg::dvec3( 0., -cDistance, 0. );
    lookat.center = vsg::dvec3( 0., 0., 0. );
    lookat.up = vsg::dvec3( 0., 0., 1. );
}


bool isNear( const vsg::dvec3& vec, const vsg::dvec3& expected, double tol )
{
    return vsg::length( vec-expected ) < tol;
}


BufferString describe( const vsg::dvec3& vec )
{
    BufferString ret( "(" );
    ret.add( vec.x ).add( "," ).add( vec.y ).add( "," ).add( vec.z );
    return ret.add( ")" );
}

} // namespace


static bool testTurn()
{
    vsg::ref_ptr<vsg::Camera> camera = createCamera();
    vsg::ref_ptr<vsg::LookAt> lookat = camera->viewMatrix.cast<vsg::LookAt>();
    vsg::ref_ptr<vsgGeo::Trackball> trackball =
					vsgGeo::Trackball::create( camera );
    Moves moves;
    trackball->setMovementCallback(
	[&moves]( float hor, float vert, float dist, bool step )
	{ moves.nrcalls_++; } );

    trackball->turn( cPi/2., true );
    const vsg::dvec3 zerovec( 0., 0., 0. );
    mRunStandardTestWithError( isNear(lookat->eye,vsg::dvec3(cDistance,0.,0.),
				      1e-6) &&
			       isNear(lookat->center,zerovec,1e-9) &&
			       isNear(lookat->up,vsg::dvec3(0.,0.,1.),1e-6),
			       "Camera turn: A horizontal turn goes around the "
			       "up direction", describe(lookat->eye) );

    resetCamera( *lookat );
    trackball->turn( 0.3, false );
    const vsg::dvec3 tilted( 0., -cDistance*std::cos(0.3),
			     cDistance*std::sin(0.3) );
    const vsg::dvec3 tiltedup( 0., std::sin(0.3), std::cos(0.3) );
    mRunStandardTestWithError( isNear(lookat->eye,tilted,1e-6) &&
			       isNear(lookat->up,tiltedup,1e-6),
			       "Camera turn: A tilt moves the camera up and "
			       "turns the up direction along",
			       describe(lookat->eye) );

    resetCamera( *lookat );
    trackball->changeDistance( 0.5 );
    mRunStandardTestWithError( isNear(lookat->eye,
				      vsg::dvec3(0.,-1.5*cDistance,0.),1e-6),
			       "Camera turn: A positive factor moves the "
			       "camera away from the center",
			       describe(lookat->eye) );

    trackball->changeDistance( -2. );
    const double distance = vsg::length( lookat->eye-lookat->center );
    mRunStandardTestWithError( distance>0. && distance<0.1 &&
			       lookat->eye.y<0.,
			       "Camera turn: The distance stays positive for "
			       "a large negative factor",
			       BufferString("Distance: ",distance) );
    mRunStandardTest( moves.nrcalls_==0,
		      "Camera turn: Moving the camera like the thumbwheels is "
		      "not reported" );
    return true;
}


static bool testReports()
{
    vsg::ref_ptr<vsg::Camera> camera = createCamera();
    vsg::ref_ptr<vsg::LookAt> lookat = camera->viewMatrix.cast<vsg::LookAt>();
    vsg::ref_ptr<vsgGeo::Trackball> trackball =
					vsgGeo::Trackball::create( camera );
    Moves moves;
    trackball->setMovementCallback(
	[&moves]( float hor, float vert, float dist, bool step )
	{
	    moves.nrcalls_++;
	    moves.hor_ = hor;
	    moves.vert_ = vert;
	    moves.dist_ = dist;
	    moves.step_ = step;
	} );

    const vsg::dvec3 axes[] = { vsg::dvec3( 1., 0., 0. ),
				vsg::normalize( vsg::dvec3(1.,1.,0.) ),
				vsg::normalize( vsg::dvec3(-1.,2.,0.5) ) };
    for ( const vsg::dvec3& axis : axes )
    {
	resetCamera( *lookat );
	moves = Moves();
	trackball->rotate( 0.05, axis );
	const vsg::dvec3 rotatedeye = lookat->eye;
	const float hor = moves.hor_;
	const float vert = moves.vert_;
	mRunStandardTestWithError( moves.nrcalls_==1 &&
				   (std::abs(hor)>1e-4f ||
				    std::abs(vert)>1e-4f) &&
				   std::abs(moves.dist_)<1e-6f && !moves.step_,
				   "Camera movement: A rotation is reported",
				   BufferString("Calls: ",moves.nrcalls_)
				    .add(", horizontal: ").add(hor)
				    .add(", vertical: ").add(vert) );

	resetCamera( *lookat );
	trackball->turn( -hor, true );
	trackball->turn( -vert, false );
	mRunStandardTestWithError( isNear(lookat->eye,rotatedeye,0.05),
				   "Camera movement: Turning the camera back "
				   "with the reported angles gives the same "
				   "position",
				   describe(lookat->eye).add(" and ")
				    .add(describe(rotatedeye)) );
    }

    resetCamera( *lookat );
    moves = Moves();
    trackball->zoom( 0.1 );
    mRunStandardTestWithError( moves.nrcalls_==1 &&
			       std::abs(moves.dist_+0.1f)<1e-5f &&
			       std::abs(moves.hor_)<1e-6f &&
			       std::abs(moves.vert_)<1e-6f,
			       "Camera movement: A zoom is reported as the "
			       "relative change of the distance",
			       BufferString("Distance: ",moves.dist_) );

    trackball->setMovementCallback( nullptr );
    moves = Moves();
    trackball->zoom( 0.1 );
    mRunStandardTest( moves.nrcalls_==0,
		      "Camera movement: Nothing is reported without a "
		      "callback" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testTurn() ||
	 !testReports() )
	return 1;

    return 0;
}
