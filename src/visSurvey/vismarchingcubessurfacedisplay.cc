/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismarchingcubessurfacedisplay.cc,v 1.21 2009-05-28 19:25:20 cvskris Exp $";

#include "vismarchingcubessurfacedisplay.h"

#include "arrayndimpl.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "executor.h"
#include "iopar.h"
#include "marchingcubes.h"
#include "marchingcubeseditor.h"
#include "randcolor.h"
#include "position.h"
#include "survinfo.h"
#include "visboxdragger.h"
#include "viscoord.h"
#include "visdragger.h"
#include "visellipsoid.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismarchingcubessurface.h"
#include "visinvisiblelinedragger.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "vispolyline.h"
#include "visshape.h"

#define mKernelSize 11

mCreateFactoryEntry( visSurvey::MarchingCubesDisplay );

namespace visSurvey
{

MarchingCubesDisplay::MarchingCubesDisplay()
    : VisualObjectImpl(true)
    , emsurface_( 0 )
    , displaysurface_( 0 )
{
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

    mTryAlloc( emsurface_, EM::MarchingCubesSurface( EM::EMM() ) );

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
    
    return true;
}


EM::ObjectID MarchingCubesDisplay::getEMID() const
{ return emsurface_ ? emsurface_->id() : -1; }


SurveyObject::AttribFormat MarchingCubesDisplay::getAttributeFormat(int) const
{ return SurveyObject::RandomPos; }


int MarchingCubesDisplay::nrAttribs() const
{ return 1; }


bool MarchingCubesDisplay::canAddAttrib(int) const
{ return false; }


bool MarchingCubesDisplay::canRemoveAttrib() const
{ return false; }


bool MarchingCubesDisplay::canHandleColTabSeqTrans(int) const
{ return false; }


const ColTab::MapperSetup*
MarchingCubesDisplay::getColTabMapperSetup( int attrib, int version ) const
{
    return !attrib && !version && displaysurface_
	? displaysurface_->getShape()->getDataMapper()
	: 0;
}


void MarchingCubesDisplay::setColTabMapperSetup( int attrib,
	const ColTab::MapperSetup& setup, TaskRunner* )
{
    if ( !attrib )
	displaysurface_->getShape()->setDataMapper( setup );
}


const ColTab::Sequence*
MarchingCubesDisplay::getColTabSequence( int attrib ) const
{
    return !attrib && displaysurface_
	? displaysurface_->getShape()->getDataSequence()
	: 0;
}


void MarchingCubesDisplay::setColTabSequence( int attrib,
			      const ColTab::Sequence& seq, TaskRunner* )
{
    if ( !attrib && displaysurface_ )
	displaysurface_->getShape()->setDataSequence( seq );
}


bool MarchingCubesDisplay::canSetColTabSequence() const
{ return true; }


void MarchingCubesDisplay::setSelSpec( int attrib, const Attrib::SelSpec& spec )
{
    if ( !attrib )
	selspec_ = spec;
}


const Attrib::SelSpec* MarchingCubesDisplay::getSelSpec( int attrib ) const
{
    return attrib ? 0 : &selspec_;
}


void MarchingCubesDisplay::getRandomPos( DataPointSet& dps,
					 TaskRunner* tr ) const
{
    if ( displaysurface_ )
	displaysurface_->getShape()->getAttribPositions( dps, tr );
}


void MarchingCubesDisplay::setRandomPosData( int attrib,
				 const DataPointSet* dps, TaskRunner* tr )
{
    if ( !attrib && dps && displaysurface_ )
	displaysurface_->getShape()->setAttribData( *dps, tr );
}



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
	    displaysurface_->removeSwitch();
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

    displaysurface_->touch( !onlyshape );
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
}


void MarchingCubesDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaysurface_ ) displaysurface_->setRightHandSystem( yn );
}


visBase::Transformation* MarchingCubesDisplay::getDisplayTransformation()
{ return displaysurface_ ? displaysurface_->getDisplayTransformation() : 0; }


}; // namespace visSurvey
