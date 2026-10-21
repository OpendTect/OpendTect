/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 dGBVis::Transformation must give the same results as
 visBase::Transformation. No GPU needed.

-*/

#include "testprog.h"

#include "visvsgtransform.h"
#include "thread.h"
#include "vistransform.h"

#include <vsg/nodes/AbsoluteTransform.h>
#include <vsg/nodes/MatrixTransform.h>

#include <random>

namespace
{

using VBTrans = visBase::Transformation;
using DVTrans = dGBVis::Transformation;

bool isClose( const Coord3& a, const Coord3& b, const Coord3& ref,
	      double releps=1e-12 )
{
    const double scale = mMAX( 1.0, mMAX(double(ref.abs()),
				mMAX(double(a.abs()),double(b.abs()))) );
    return a.distTo(b) <= releps*scale;
}


struct TransformPair
{
    RefMan<VBTrans>	vb_ = VBTrans::create();
    RefMan<DVTrans>	dv_ = DVTrans::create();
};


#define mCompareFn( fn ) \
{ \
    Coord3 vbres, dvres; \
    pair.vb_->fn( inp, vbres ); \
    pair.dv_->fn( inp, dvres ); \
    if ( !isClose(vbres,dvres,inp) ) \
    { \
	msg.set( #fn ).add( " of " ).add( inp.toString() ) \
	   .add( ": visBase " ).add( vbres.toString() ) \
	   .add( ", dGBVis " ).add( dvres.toString() ); \
	return false; \
    } \
}

bool compareAll( const TransformPair& pair, const Coord3& inp,
		 BufferString& msg )
{
    mCompareFn( transform )
    mCompareFn( transformBack )
    mCompareFn( transformDir )
    mCompareFn( transformBackDir )
    mCompareFn( transformSize )
    mCompareFn( transformBackSize )
    mCompareFn( transformNormal )
    mCompareFn( transformBackNormal )
    return true;
}


bool comparePoints( const TransformPair& pair, std::mt19937& rng,
		    double range, BufferString& msg )
{
    std::uniform_real_distribution<double> dist( -range, range );
    for ( int idx=0; idx<10000; idx++ )
    {
	const Coord3 inp( dist(rng), dist(rng), dist(rng) );
	if ( !compareAll(pair,inp,msg) )
	    return false;
    }

    return true;
}


class ChangeCounter : public CallBacker
{
public:
    void		changeCB( CallBacker* )		{ nrchanges_++; }
    int			nrchanges_ = 0;
};

} // namespace


static bool testSetMatrix()
{
    std::mt19937 rng( 99 );
    std::uniform_real_distribution<double> unit( -1.0, 1.0 );
    BufferString msg;
    for ( int itest=0; itest<20; itest++ )
    {
	TransformPair pair;
	const Coord3 trans( unit(rng)*1e6, unit(rng)*1e6, unit(rng)*1e3 );
	const Coord3 rotvec( unit(rng), unit(rng), unit(rng) );
	const double angle = unit(rng) * M_PI;
	const Coord3 scale( 0.5+unit(rng)*0.4, 1.5+unit(rng),
			    -0.2+unit(rng)*0.1 );
	pair.vb_->setMatrix( trans, rotvec, angle, scale );
	pair.dv_->setMatrix( trans, rotvec, angle, scale );
	if ( itest%2 )
	{
	    const Coord3 sovec( unit(rng), unit(rng), unit(rng) );
	    const double soangle = unit(rng) * M_PI;
	    pair.vb_->setScaleOrientation( sovec, soangle );
	    pair.dv_->setScaleOrientation( sovec, soangle );
	}

	mRunStandardTestWithError( comparePoints(pair,rng,1e3,msg) &&
				   comparePoints(pair,rng,1e7,msg),
				   "setMatrix: all transform functions",
				   msg );

	Coord3 vbvec, dvvec; double vbangle, dvangle;
	pair.vb_->getRotation( vbvec, vbangle );
	pair.dv_->getRotation( dvvec, dvangle );
	mRunStandardTest(
	    isClose(pair.vb_->getTranslation(),pair.dv_->getTranslation(),
		    trans) &&
	    isClose(pair.vb_->getScale(),pair.dv_->getScale(),scale) &&
	    isClose(vbvec,dvvec,Coord3(1,1,1)) &&
	    mIsEqual(vbangle,dvangle,1e-12),
	    "setMatrix: getters" );
    }

    return true;
}


static bool testSetA()
{
    std::mt19937 rng( 5 );
    std::uniform_real_distribution<double> unit( -1.0, 1.0 );
    BufferString msg;

    {
	// As SceneTransformManager::computeUTM2DisplayTransform()
	TransformPair pair;
	const double startx = 605000., starty = 6070000.;
	const double zfactor = -2.3, zmidpt = 1.2;
	pair.vb_->setA( 1, 0, 0, -startx,
			0, 1, 0, -starty,
			0, 0, zfactor, -zfactor*zmidpt,
			0, 0, 0, 1 );
	pair.dv_->setA( 1, 0, 0, -startx,
			0, 1, 0, -starty,
			0, 0, zfactor, -zfactor*zmidpt,
			0, 0, 0, 1 );
	mRunStandardTestWithError( comparePoints(pair,rng,1e7,msg),
				   "setA: UTM to display", msg );
    }

    for ( int itest=0; itest<20; itest++ )
    {
	double a[16];
	for ( int idx=0; idx<16; idx++ )
	    a[idx] = unit( rng );
	a[0] += 2.; a[5] += 2.; a[10] += 2.;
	a[3] *= 1e5; a[7] *= 1e5; a[11] *= 1e3;
	if ( itest%2 )
	    { a[12] *= 1e-4; a[13] *= 1e-4; a[14] *= 1e-4; a[15] = 1.; }
	else
	    { a[12] = a[13] = a[14] = 0.; a[15] = 1.; }

	TransformPair pair;
	pair.vb_->setA( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
			a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15] );
	pair.dv_->setA( a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
			a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15] );
	mRunStandardTestWithError( comparePoints(pair,rng,1e3,msg),
				   itest%2 ? "setA: projective matrix"
					   : "setA: affine matrix", msg );
    }

    return true;
}


static bool testNullTransformation()
{
    const Coord3 inp( 605123.5, 6070123.25, 1.125 );
    Coord3 out;
    DVTrans::transform( nullptr, inp, out );
    mRunStandardTest( out==inp, "Null transformation: Coord3" );

    vsg::dvec3 dout;
    DVTrans::transformBack( nullptr, inp, dout );
    mRunStandardTest( dout.x==inp.x_ && dout.y==inp.y_ && dout.z==inp.z_,
		      "Null transformation: Coord3 to dvec3" );

    Coord3 inplace( inp );
    DVTrans::transformDir( nullptr, inplace );
    mRunStandardTest( inplace==inp, "Null transformation: in place" );
    return true;
}


static bool testChangeNotifier()
{
    ChangeCounter counter;
    RefMan<DVTrans> trans = DVTrans::create();
    trans->transformchange.notify( mCB(&counter,ChangeCounter,changeCB) );

    trans->setTranslation( Coord3(1,2,3) );
    trans->setScale( Coord3(1,1,2) );
    trans->setA( 1,0,0,5, 0,1,0,6, 0,0,1,7, 0,0,0,1 );
    mRunStandardTest( counter.nrchanges_==3,
		      "transformchange on every change" );

    const auto* mt = dynamic_cast<const vsg::MatrixTransform*>(
					trans->vsgNode(true) );
    mRunStandardTest( mt && mt->matrix==trans->getMatrix(),
		      "Node matrix follows" );

    trans->setAbsoluteReferenceFrame();
    trans->setTranslation( Coord3(4,5,6) );
    const auto* abst = dynamic_cast<const vsg::AbsoluteTransform*>(
					trans->vsgNode(true) );
    mRunStandardTest( abst && abst->matrix==trans->getMatrix(),
		      "Absolute reference frame node follows" );

    trans->transformchange.remove( mCB(&counter,ChangeCounter,changeCB) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );

    if ( !testSetMatrix() ||
	 !testSetA() ||
	 !testNullTransformation() ||
	 !testChangeNotifier() )
	return 1;

    return 0;
}
