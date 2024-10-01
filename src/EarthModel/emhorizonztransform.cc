/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emhorizonztransform.h"

#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "iopar.h"
#include "survinfo.h"
#include "sorting.h"
#include "zdomain.h"
#include "zvalseriesimpl.h"

namespace EM
{

static const ZDomain::Def& getZDom()
{
    return SI().zIsTime() ? ZDomain::Def::get( "Time-Flattened" )
			  : ZDomain::Def::get( "Depth-Flattened" );
}


const ZDomain::Info& flattenedZDomain()
{
    IOPar flatiop;
    flatiop.set( ZDomain::sKey(), getZDom().key() );
    return *ZDomain::get( flatiop );
}


// HorizonZTransform

HorizonZTransform::HorizonZTransform()
    : ZAxisTransform(ZDomain::SI(),getZDom())
    , depthrange_(Interval<float>::udf())
    , change_(this)
{}


HorizonZTransform::~HorizonZTransform()
{
    detachAllNotifiers();
}


bool HorizonZTransform::isOK() const
{
    if ( !horizon_ || depthrange_.isUdf() )
	return false;

    return ZAxisTransform::isOK();
}


ZSampling HorizonZTransform::getModelZSampling() const
{
    return horizon_->getZRange();
}


void HorizonZTransform::setHorizon( const Horizon& hor )
{
    if ( horizon_ )
	mDetachCB( horizon_->change, HorizonZTransform::horChangeCB );

    horizon_ = &hor;
    mAttachCB( horizon_->change, HorizonZTransform::horChangeCB );

    fromzdomaininfo_.setID( horizon_->multiID() );
    tozdomaininfo_.setID( horizon_->multiID() );

    horchanged_ = true;
    calculateHorizonRange();
}


bool HorizonZTransform::isReferenceHorizon( const MultiID& horid,
					    float& refz ) const
{
    if ( !horizon_ || horizon_->multiID() != horid )
	return false;

    refz = 0.f;
    return true;
}


void HorizonZTransform::setFlatZValue( float flatzval )
{
    flatzval_ = flatzval;
}


void HorizonZTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );

    if ( horizon_ )
	par.set( sKeyHorizonID(), horizon_->multiID() );
}


bool HorizonZTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    MultiID mid;
    if ( !par.get( sKeyHorizonID(),mid) )
	return true;

    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
    if ( !emobj )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader( mid );
	if ( !loader || !loader->execute() ) return false;

	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }

    mDynamicCastGet(EM::Horizon*,hor,emobj.ptr())
    if ( !hor )
	return false;

    setHorizon( *hor );
    return true;
}


void HorizonZTransform::transformTrc( const TrcKey& trckey,
				      const SamplingData<float>& sd,
				      int sz, float* res ) const
{
    doTransform( trckey, sd, fromZDomainInfo(), sz, res );
}


void HorizonZTransform::transformTrcBack( const TrcKey& trckey,
					  const SamplingData<float>& sd,
					  int sz, float* res ) const
{
    doTransform( trckey, sd, toZDomainInfo(), sz, res );
}


void HorizonZTransform::doTransform( const TrcKey& trckey,
				     const SamplingData<float>& sd,
				     const ZDomain::Info& sdzinfo,
				     int sz, float* res ) const
{
    float top, bottom;
    const bool hastopbot = getTopBottom( trckey, top, bottom );
    if ( sd.isUdf() || !horizon_ || !hastopbot )
    {
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const RegularZValues zvals( sd, sz, sdzinfo );
    if ( zvals.zDomainInfo().def_ == getZDom() )
	for ( int idx=0; idx<sz; idx++ )
	    res[idx] = zvals[idx] + top - flatzval_;
    else
	for ( int idx=0; idx<sz; idx++ )
	    res[idx] = zvals[idx] - top + flatzval_;
}


ZSampling HorizonZTransform::getWorkZSampling( const ZSampling& zsamp,
					       const ZDomain::Info& from,
					       const ZDomain::Info& to ) const
{
    ZSampling ret = zsamp;
    if ( to == from )
	return ret;

    if ( horchanged_ )
	mSelf().calculateHorizonRange();

    if ( !isOK() )
	return ZSampling::udf();

    if ( to.def_ == getZDom() )
    {
	ret.start_ -= depthrange_.stop_;
	ret.stop_ -= depthrange_.start_;
	ret.shift( flatzval_ );
    }

    return ret;
}


void HorizonZTransform::horChangeCB(CallBacker*)
{
    horchanged_ = true;

    if ( horizon_ && !horizon_->hasBurstAlert() )
	change_.trigger();
}


void HorizonZTransform::calculateHorizonRange()
{
    if ( !horizon_ )
	return;

    PtrMan<EMObjectIterator> iterator = horizon_->createIterator();
    if ( !iterator )
	return;

    bool isset = false;

    EM::PosID pid = iterator->next();
    while ( pid.isValid() )
    {
	const float depth = (float) horizon_->getPos( pid ).z;
	if ( !mIsUdf( depth ) )
	{
	    if ( isset ) depthrange_.include( depth, false );
	    else { depthrange_.start_ = depthrange_.stop_ = depth; isset=true; }
	}

	pid = iterator->next();
    }

    horchanged_ = false;
}


bool HorizonZTransform::getTopBottom( const TrcKey& trckey, float& top,
				      float& bottom ) const
{
    mDynamicCastGet(const Horizon3D*,hor3d,horizon_.ptr())
    mDynamicCastGet(const Horizon2D*,hor2d,horizon_.ptr())
    TypeSet<float> depths;
    TrcKey hortrckey;
    if ( trckey.geomSystem() == horizon_->getSurveyID() )
	hortrckey = trckey;
    else
    {
	const Pos::GeomID gid = trckey.geomID();
	ConstRefMan<Survey::Geometry> keysurv = Survey::GM().getGeometry( gid );
	if ( !keysurv )
	    return false;

	const Coord crd = keysurv->toCoord( trckey );
	if ( crd.isUdf() )
	    return false;

	if ( hor3d )
	{
	    const Pos::GeomID horgid =
				TrcKey::gtGeomID( horizon_->getSurveyID() );
	    ConstRefMan<Survey::Geometry> horsurv =
					    Survey::GM().getGeometry( horgid );
	    hortrckey = horsurv->getTrace( crd, horsurv->averageTrcDist() * 3 );
	}
	else if ( hor2d )
	{
	    float bestdist2 = mUdf(float);
	    for ( int idx=0; idx<hor2d->geometry().nrLines(); idx++ )
	    {
		ConstRefMan<Survey::Geometry> horsurv =
		    Survey::GM().getGeometry( hor2d->geometry().geomID(idx) );
		if ( !horsurv ) continue;

		const TrcKey tmptrckey =
		    horsurv->getTrace( crd, horsurv->averageTrcDist() * 3 );

		const Coord trccrd = horsurv->toCoord( tmptrckey );
		if ( trccrd.isDefined() )
		{
		    const float dist2 = (float) crd.sqDistTo( trccrd );
		    if ( mIsUdf(bestdist2) || dist2<bestdist2 )
		    {
			hortrckey = tmptrckey;
			bestdist2 = dist2;
		    }
		}
	    }
	}
    }

    float depth = horizon_->getZ( hortrckey );
    if ( mIsUdf( depth ) && hor3d )
    {
	const BinID bid = hortrckey.position();
	const Geometry::BinIDSurface* geom =
		    hor3d->geometry().geometryElement();
	depth = (float)geom->computePosition(Coord(bid.inl(),bid.crl()) ).z;
    }

    if ( !mIsUdf(depth) )
	depths += depth;

    if ( depths.size()>1 )
	sort_array( depths.arr(), depths.size() );
    else if ( !depths.size() )
	return false;

    top = depths[0];
    bottom = depths[depths.size()-1];

    return true;
}

} // namespace EM
