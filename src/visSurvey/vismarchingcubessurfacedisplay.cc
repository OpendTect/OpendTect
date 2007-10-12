/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismarchingcubessurfacedisplay.cc,v 1.9 2007-10-12 21:30:28 cvsyuancheng Exp $";

#include "vismarchingcubessurfacedisplay.h"

#include "arrayndimpl.h"
#include "iopar.h"
#include "executor.h"
#include "marchingcubeseditor.h"
#include "survinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "marchingcubes.h"
#include "randcolor.h"
#include "visboxdragger.h"
#include "visdragger.h"
#include "visellipsoid.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vismarchingcubessurface.h"

#define mKernelSize 11
#define mHalfKernel 5

mCreateFactoryEntry( visSurvey::MarchingCubesDisplay );

namespace visSurvey
{

MarchingCubesDisplay::MarchingCubesDisplay()
    : VisualObjectImpl(true)
    , emsurface_( 0 )
    , initialdragger_( visBase::BoxDragger::create() )
    , displaysurface_( 0 )
    , surfaceeditor_( 0 )
    , eventcatcher_( 0 )
    , factordragger_( 0 )
    , initialellipsoid_( visBase::Ellipsoid::create() )
    , minsampleinlsz_( 0 )						
    , minsamplecrlsz_( 0 )						
    , minsamplezsz_( 0 )
    , previoussample_( false )			
{
    initialdragger_->ref();
    initialdragger_->turnOn( true );
    addChild( initialdragger_->getInventorNode() );

    initialellipsoid_->ref();
    initialellipsoid_->removeSwitch();
    initialellipsoid_->setMaterial( 0 );
    addChild( initialellipsoid_->getInventorNode() );

    getMaterial()->setAmbience( 0.3 );

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

    Coord3 center( ((float) cs.hrg.start.inl+cs.hrg.stop.inl)/2,
	    	   ((float) cs.hrg.start.crl+cs.hrg.stop.crl)/2,
		   cs.zrg.center() );

    Coord3 width( cs.hrg.stop.inl-cs.hrg.start.inl,
	    	  cs.hrg.stop.crl-cs.hrg.start.crl,	                                          cs.zrg.width() );

    initialdragger_->setCenter( center );
    initialdragger_->setWidth( width );
    initialellipsoid_->setCenterPos( center );
    initialellipsoid_->setWidth( width );

    initialdragger_->setSpaceLimits(
	    Interval<float>( sics.hrg.start.inl, sics.hrg.stop.inl ),
	    Interval<float>( sics.hrg.start.crl, sics.hrg.stop.crl ),
	    Interval<float>( sics.zrg.start, sics.zrg.stop ) );

    minsampleinlsz_ = width.x/4;
    minsamplecrlsz_ = width.y/4;
    minsamplezsz_ = width.z/4;
    previoussample_ = cs;

    initialdragger_->finished.notify( 
	    mCB(this, MarchingCubesDisplay, draggerMovedCB) );

    initialdragger_->motion.notify(
	    mCB(this, MarchingCubesDisplay, draggerMovingCB) );

    setColor( getRandomColor( false ) );
}


MarchingCubesDisplay::~MarchingCubesDisplay()
{
    if ( emsurface_ ) emsurface_->unRef();
    
    if ( displaysurface_ )
    {
	removeChild( displaysurface_->getInventorNode() );
	displaysurface_->unRef();
    }

    if ( factordragger_ )
    {
	factordragger_->unRef();
	factordragger_->motion.remove(mCB(this,MarchingCubesDisplay,
		    		      factorDrag));
    }

    delete surfaceeditor_;
    setSceneEventCatcher(0);
    removeInitialDragger();
}


void MarchingCubesDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,MarchingCubesDisplay,pickCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(
		mCB(this,MarchingCubesDisplay,pickCB) );
	eventcatcher_->ref();
    }
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
    displaysurface_->setSelectable( false );
    displaysurface_->setRightHandSystem( righthandsystem_ );
    addChild( displaysurface_->getInventorNode() );

    if ( displaysurface_->getMaterial() )
	getMaterial()->setFrom( *displaysurface_->getMaterial() );

    displaysurface_->setMaterial( 0 );
    emsurface_->setPreferredColor( getMaterial()->getColor() );
    
    removeInitialDragger();
    return true;
}


EM::ObjectID MarchingCubesDisplay::getEMID() const
{ return emsurface_ ? emsurface_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool MarchingCubesDisplay::setEMID( const EM::ObjectID& emid )
{
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

    updateVisFromEM( false );
    return true;
}


void MarchingCubesDisplay::updateVisFromEM( bool onlyshape )
{
    if ( !onlyshape || !displaysurface_ )
    {
	getMaterial()->setColor( emsurface_->preferredColor() );
	if ( !emsurface_->name().isEmpty() )
	    setName( emsurface_->name() );
	else setName( "<New body>" );

	if ( !displaysurface_ )
	{
	    displaysurface_ = visBase::MarchingCubesSurface::create();
	    displaysurface_->ref();
	    displaysurface_->setMaterial( 0 );
	    displaysurface_->setSelectable( false );
	    displaysurface_->setRightHandSystem( righthandsystem_ );
	    addChild( displaysurface_->getInventorNode() );
	}

	displaysurface_->setScales(
		SamplingData<float>(emsurface_->inlSampling()),
		SamplingData<float>(emsurface_->crlSampling()),
				    emsurface_->zSampling() );

	displaysurface_->setSurface( emsurface_->surface() );
	displaysurface_->turnOn( true );
    }

    if ( emsurface_ && !emsurface_->surface().isEmpty() )
	removeInitialDragger();

    displaysurface_->touch();
}


void MarchingCubesDisplay::showManipulator( bool yn )
{
    if ( initialdragger_ )
    	initialdragger_->turnOn( yn );
    
    if ( factordragger_ )
	factordragger_->turnOn( yn );
} 


bool MarchingCubesDisplay::isManipulatorShown() const
{
    if ( initialdragger_ )
	return initialdragger_->isOn();
    else if (  factordragger_ )
	return  factordragger_->isOn();
    else
	return false;
}


bool MarchingCubesDisplay::hasInitialShape()
{ 
    return ( initialdragger_ && !displaysurface_ ) ||
       	   ( initialdragger_ && displaysurface_->getSurface()->isEmpty() ); 
}


bool MarchingCubesDisplay::createInitialBody( bool allowswap )
{
    if ( !initialdragger_ )
	return false;

    if ( !allowswap )
	return true;
    
    const Coord3 center = initialdragger_->center();
    const Coord3 width = initialdragger_->width();

    CubeSampling cs( true );
    cs.zrg = StepInterval<float>(
	    center.z-width.z/2, center.z+width.z/2, SI().zStep() );
    cs.hrg.start.inl = mNINT(center.x-width.x/2);
    cs.hrg.stop.inl = mNINT(center.x+width.x/2)+1;
    cs.hrg.start.crl = mNINT(center.y-width.y/2);
    cs.hrg.stop.crl = mNINT(center.y+width.y/2)+1;

    const int xsz = cs.nrInl();
    const int ysz = cs.nrCrl();
    const int zsz = cs.nrZ();
    const float hxsz = (float)xsz/2-0.5;
    const float hysz = (float)ysz/2-0.5;
    const float hzsz = (float)zsz/2-0.5;
    
    PtrMan<Array3DImpl<float> > array = new Array3DImpl<float> (xsz, ysz, zsz);
    if ( !array || !array->isOK() )
	return false;
    
    for ( int idx=0; idx<xsz; idx++ )
    {
	for ( int idy=0; idy<ysz; idy++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
	    {
		const float diffx = ((float)idx-hxsz)/hxsz;
		const float diffy = ((float)idy-hysz)/hysz;
		const float diffz = ((float)idz-hzsz)/hzsz;
		const float diff = sqrt( diffx*diffx+diffy*diffy+diffz*diffz );
		array->set( idx, idy, idz, diff );
	    }
	}
    }

    emsurface_->setInlSampling(
	    SamplingData<int>(cs.hrg.start.inl,cs.hrg.step.inl) );
    emsurface_->setCrlSampling(
	    SamplingData<int>(cs.hrg.start.crl,cs.hrg.step.crl) );

    emsurface_->setZSampling( SamplingData<float>( cs.zrg.start, cs.zrg.step ));
    
    ::MarchingCubesSurface& mcs = emsurface_->surface();
    mcs.removeAll();
    mcs.setVolumeData( 0, 0, 0, *array, 1 );

    array = 0;

    updateVisFromEM( false );

    return true;
}


void MarchingCubesDisplay::removeInitialDragger()
{
    if ( initialellipsoid_ )
    {
	removeChild( initialellipsoid_->getInventorNode() );
	initialellipsoid_->unRef();
	initialellipsoid_ = 0;
    }
    
    if ( !initialdragger_ )
	return;
    
    initialdragger_->finished.remove(
	    mCB(this, MarchingCubesDisplay, draggerMovedCB) );

    initialdragger_->motion.remove(
	    mCB(this, MarchingCubesDisplay, draggerMovingCB) );

    removeChild( initialdragger_->getInventorNode() );
    initialdragger_->unRef();
    initialdragger_ = 0;
}


void MarchingCubesDisplay::draggerMovedCB( CallBacker* )
{
    if ( !initialdragger_ )
	return;

    const Coord3 center = initialdragger_->center();
    const Coord3 width = initialdragger_->width();

    CubeSampling cs( true );
    cs.zrg = StepInterval<float>(
	    center.z-width.z/2, center.z+width.z/2, SI().zStep() );
    cs.hrg.start.inl = mNINT(center.x-width.x/2);
    cs.hrg.stop.inl = mNINT(center.x+width.x/2)+1;
    cs.hrg.start.crl = mNINT(center.y-width.y/2);
    cs.hrg.stop.crl = mNINT(center.y+width.y/2)+1;
    cs.snapToSurvey();

    previoussample_ = cs;

    Coord3 newcenterpos( cs.hrg.inlRange().center(), 
	    		 cs.hrg.crlRange().center(), cs.zrg.center() );
    Coord3 newwidth( cs.hrg.inlRange().width(false), 
	    	     cs.hrg.crlRange().width(false), cs.zrg.width(false) );
    initialdragger_->setCenter( newcenterpos );
    initialdragger_->setWidth( newwidth );
    initialellipsoid_->setCenterPos( newcenterpos );
    initialellipsoid_->setWidth( newwidth );
}


void MarchingCubesDisplay::draggerMovingCB( CallBacker* )
{
    if ( !initialdragger_ )
	return;
    
    if ( !initialellipsoid_ )
    {
	initialellipsoid_ = visBase::Ellipsoid::create();
	initialellipsoid_->ref();
	addChild( initialellipsoid_->getInventorNode() );
    }

    Coord3 center = initialdragger_->center();
    Coord3 width = initialdragger_->width();
    
    if ( width.x<minsampleinlsz_ || width.y<minsamplecrlsz_ ||
	 width.z<minsamplezsz_ )
    {
	Coord3 centerpos( previoussample_.hrg.inlRange().center(), 
			  previoussample_.hrg.crlRange().center(),
	    		  previoussample_.zrg.center() );
    	Coord3 prevwidth( previoussample_.hrg.inlRange().width(false), 
			  previoussample_.hrg.crlRange().width(false),
    			  previoussample_.zrg.width(false) );
	
	initialdragger_->setCenter( centerpos );    
    	initialdragger_->setWidth( prevwidth );
	
	initialellipsoid_->setCenterPos( centerpos );
    	initialellipsoid_->setWidth( prevwidth );
    }
    else
    {
	previoussample_.zrg = StepInterval<float>(
		center.z-width.z/2, center.z+width.z/2, SI().zStep() );
	previoussample_.hrg.start.inl = mNINT(center.x-width.x/2);
	previoussample_.hrg.stop.inl = mNINT(center.x+width.x/2)+1;
	previoussample_.hrg.start.crl = mNINT(center.y-width.y/2);
	previoussample_.hrg.stop.crl = mNINT(center.y+width.y/2)+1;
	previoussample_.snapToSurvey();
	
	initialellipsoid_->setCenterPos( center );
    	initialellipsoid_->setWidth( width );
    }
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


NotifierAccess* MarchingCubesDisplay::materialChange()
{ return &getMaterial()->change; }


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
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    return 1;
}


void MarchingCubesDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( displaysurface_ ) displaysurface_->setDisplayTransformation( nt );
    if ( initialellipsoid_ ) initialellipsoid_->setDisplayTransformation( nt );
}


void MarchingCubesDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaysurface_ ) displaysurface_->setRightHandSystem( yn );
}


visBase::Transformation* MarchingCubesDisplay::getDisplayTransformation()
{ return displaysurface_ ? displaysurface_->getDisplayTransformation() : 0; }


void MarchingCubesDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !isOn() || isLocked() || !displaysurface_ ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( eventinfo.type!=visBase::MouseClick ||
	 eventinfo.mousebutton != visBase::EventInfo::leftMouseButton() )
	return;

    if ( eventinfo.pickedobjids.indexOf( displaysurface_->id() )==-1 )
	return;

    if ( eventinfo.pressed )
	return;

    const Coord3& wp = eventinfo.worldpickedpos;
    const BinID bid = SI().transform( wp );

    const int surfacepos[] = { emsurface_->inlSampling().nearestIndex(bid.inl),
			       emsurface_->crlSampling().nearestIndex(bid.crl),
			       emsurface_->zSampling().nearestIndex(wp.z) };

    if ( !surfaceeditor_ )
    {
	surfaceeditor_ = new MarchingCubesSurfaceEditor( 
		emsurface_->surface() );
    }

    PtrMan<Array3D<unsigned char> > kernel = 
			createKernel( mKernelSize,mKernelSize, mKernelSize );
    surfaceeditor_->setKernel( *kernel, surfacepos[0]-mHalfKernel, 
	    surfacepos[1]-mHalfKernel, surfacepos[2]-mHalfKernel );

    if ( !factordragger_ )
    {
	factordragger_ = visBase::Dragger::create();
	factordragger_->ref();
	factordragger_->setDraggerType( visBase::Dragger::Translate1D );
	    addChild( factordragger_->getInventorNode() );
	factordragger_->motion.notify(mCB(this,MarchingCubesDisplay,
		    		      factorDrag ));
    }

    factordragger_->setPos( Coord3(bid.inl, bid.crl, wp.z ) );
    startpos_ = wp.z;

    eventcatcher_->eventIsHandled();
}


void MarchingCubesDisplay::factorDrag( CallBacker* )
{
    if ( !factordragger_ )
	return;

    const float drag = startpos_-factordragger_->getPos().z;
    surfaceeditor_->setFactor( mNINT(drag*255) );
    updateVisFromEM( true );
}


Array3D<unsigned char>*
MarchingCubesDisplay::createKernel( int xsz, int ysz, int zsz ) const
{
    Array3D<unsigned char>* res = new Array3DImpl<unsigned char>(xsz,ysz,zsz);
    if ( !res || !res->isOK() )
    {
	delete res;
	return 0;
    }

    const int hxsz = xsz/2; const int hysz = ysz/2; const int hzsz = zsz/2;

    for ( int idx=0; idx<xsz; idx++ )
    {
	float xval = idx-hxsz; xval /= hxsz; xval *= xval;
	for ( int idy=0; idy<ysz; idy++ )
	{
	    float yval = idy-hysz; yval /= hysz; yval *= yval;
	    for ( int idz=0; idz<zsz; idz++ )
	    {
		float zval = idz-hzsz; zval /= hzsz; zval *= zval;

		int invdist = mNINT((1-sqrt( xval+yval+zval ))*255);
		if ( invdist<0 ) invdist = 0;
		res->set( idx, idy, idz, invdist );
	    }
	}
    }

    return res;
}


}; // namespace visSurvey
