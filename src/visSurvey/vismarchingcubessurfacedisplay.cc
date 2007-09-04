/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismarchingcubessurfacedisplay.cc,v 1.1 2007-09-04 21:26:17 cvskris Exp $";

#include "vismarchingcubessurfacedisplay.h"

#include "iopar.h"
#include "executor.h"
#include "survinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "randcolor.h"
#include "visboxdragger.h"
#include "vismaterial.h"
#include "vismarchingcubessurface.h"

mCreateFactoryEntry( visSurvey::MarchingCubesDisplay );

namespace visSurvey
{

MarchingCubesDisplay::MarchingCubesDisplay()
    : VisualObjectImpl(true)
    , emsurface_( 0 )
    , initialdragger_( visBase::BoxDragger::create() )
    , displaysurface_( 0 )
{
    initialdragger_->ref();
    initialdragger_->turnOn(true);
    addChild( initialdragger_->getInventorNode() );

    CubeSampling cs(false);
    CubeSampling sics = SI().sampling(true);
    cs.hrg.start.inl = (5*sics.hrg.start.inl+3*sics.hrg.stop.inl)/8;
    cs.hrg.start.crl = (5*sics.hrg.start.crl+3*sics.hrg.stop.crl)/8;
    cs.hrg.stop.inl = (3*sics.hrg.start.inl+5*sics.hrg.stop.inl)/8;
    cs.hrg.stop.crl = (3*sics.hrg.start.crl+5*sics.hrg.stop.crl)/8;
    cs.zrg.start = ( 5*sics.zrg.start + 3*sics.zrg.stop ) / 8;
    cs.zrg.stop = ( 3*sics.zrg.start + 5*sics.zrg.stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    float z0 = SI().zRange(true).snap( cs.zrg.start ); cs.zrg.start = z0;
    float z1 = SI().zRange(true).snap( cs.zrg.stop ); cs.zrg.stop = z1;

    initialdragger_->setCenter(
	    Coord3(((float) cs.hrg.start.inl+cs.hrg.stop.inl)/2,
		   ((float) cs.hrg.start.crl+cs.hrg.stop.crl)/2,
		   cs.zrg.center() ) );

    initialdragger_->setWidth( Coord3( cs.hrg.stop.inl-cs.hrg.start.inl,
				       cs.hrg.stop.crl-cs.hrg.start.crl,
				       cs.zrg.width() ) );

    initialdragger_->setSpaceLimits(
	    Interval<float>( sics.hrg.start.inl, sics.hrg.stop.inl ),
	    Interval<float>( sics.hrg.start.crl, sics.hrg.stop.crl ),
	    Interval<float>( sics.zrg.start, sics.zrg.stop ) );

    setColor( getRandomColor( false ) );
}


MarchingCubesDisplay::~MarchingCubesDisplay()
{
    if ( displaysurface_ )
    {
	removeChild( displaysurface_->getInventorNode() );
	displaysurface_->unRef();
    }

    if ( initialdragger_ ) initialdragger_->unRef();
    if ( emsurface_ ) emsurface_->unRef();

    setSceneEventCatcher(0);
}


bool MarchingCubesDisplay::setVisSurface(
	visBase::MarchingCubesSurface* surface )
{
    if ( displaysurface_ )
    {
	removeChild( displaysurface_->getInventorNode() );
	displaysurface_->unRef();
	displaysurface_ = 0;
    }
	
    if ( emsurface_ ) emsurface_->unRef();
    emsurface_ = 0;

    if ( !surface || !surface->getSurface() )
	return false;

    emsurface_ = new EM::MarchingCubesSurface( EM::EMM() );

    if ( !emsurface_ )
	return false;

    emsurface_->ref();
    emsurface_->setPreferredColor( getMaterial()->getColor() );

    if ( !emsurface_->isOK() ||
	 !emsurface_->setSurface( surface->getSurface() ) )
    {
	emsurface_->unRef();
	emsurface_ = 0;
	return false;
    }

    SamplingData<float> sd = surface->getScale( 0 );
    emsurface_->setInlSampling(
	    SamplingData<int>( mNINT(sd.start), mNINT(sd.step) ) );

    sd = surface->getScale( 1 );
    emsurface_->setCrlSampling(
	    SamplingData<int>( mNINT(sd.start), mNINT(sd.step) ) );

    emsurface_->setZSampling( surface->getScale( 2 ) );

    EM::EMM().addObject( emsurface_ );

    displaysurface_ = surface;

    displaysurface_->ref();
    displaysurface_->setMaterial( 0 );
    displaysurface_->setSelectable( false );
    addChild( displaysurface_->getInventorNode() );

    if ( initialdragger_ )
    {
	removeChild( initialdragger_->getInventorNode() );
	initialdragger_->unRef();
	initialdragger_ = 0;
    }

    return true;
}


EM::ObjectID MarchingCubesDisplay::getEMID() const
{ return emsurface_ ? emsurface_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool MarchingCubesDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( initialdragger_ )
    {
	removeChild( initialdragger_->getInventorNode() );
	initialdragger_->unRef();
	initialdragger_ = 0;
    }

    if ( emsurface_ )
	emsurface_->unRef();

    emsurface_ = 0;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::MarchingCubesSurface*, emmcsurf, emobject.ptr() );
    if ( !emmcsurf )
    {
	if ( displaysurface_ ) displaysurface_->turnOn( false );
	return false;
    }

    emsurface_ = emmcsurf;
    emsurface_->ref();

    getMaterial()->setColor( emsurface_->preferredColor() );
    if ( !emsurface_->name().isEmpty() )
	setName( emsurface_->name() );

    if ( !displaysurface_ )
    {
	displaysurface_ = visBase::MarchingCubesSurface::create();
	displaysurface_->ref();
	displaysurface_->setMaterial( 0 );
	displaysurface_->setSelectable( false );
	addChild( displaysurface_->getInventorNode() );
    }

    displaysurface_->setScales( SamplingData<float>(emsurface_->inlSampling()),
				SamplingData<float>(emsurface_->inlSampling()),
				emsurface_->zSampling() );

    displaysurface_->setSurface( emsurface_->surface() );
    displaysurface_->turnOn( true );

    return true;
}


MultiID MarchingCubesDisplay::getMultiID() const
{
    return emsurface_ ? emsurface_->multiID() : MultiID();
}


void MarchingCubesDisplay::setColor( Color nc )
{
    if ( emsurface_ ) emsurface_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


Color MarchingCubesDisplay::getColor() const
{ return getMaterial()->getColor(); }


void MarchingCubesDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID(), getMultiID() );
}


int MarchingCubesDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	const EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    return 1;
}


void MarchingCubesDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( displaysurface_ ) displaysurface_->setDisplayTransformation( nt );
}


visBase::Transformation* MarchingCubesDisplay::getDisplayTransformation()
{ return displaysurface_ ? displaysurface_->getDisplayTransformation() : 0; }


}; // namespace visSurvey
