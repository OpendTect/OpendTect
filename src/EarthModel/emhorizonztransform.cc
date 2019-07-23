/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emhorizonztransform.h"

#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "iopar.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "sorting.h"
#include "zdomain.h"


static const ZDomain::Def& getZDom()
{
    return SI().zIsTime() ? ZDomain::Def::get( "Time-Flattened" )
			  : ZDomain::Def::get( "Depth-Flattened" );
}


EM::HorizonZTransform::HorizonZTransform()
    : ZAxisTransform(ZDomain::SI(),getZDom())
    , horizon_( 0 )
    , horchanged_( false )
    , change_( this )
{}


EM::HorizonZTransform::~HorizonZTransform()
{
    if ( horizon_ )
    {
	const_cast<Horizon*>(horizon_)->objectChanged().remove(
		mCB(this,HorizonZTransform,horChangeCB) );
	horizon_->unRef();
    }
}


void EM::HorizonZTransform::setHorizon( const Horizon& hor )
{
    if ( horizon_ )
    {
	const_cast<Horizon*>(horizon_)
	    ->objectChanged().remove( mCB(this,HorizonZTransform,horChangeCB) );
	horizon_->unRef();
    }

    horizon_ = &hor;
    horizon_->ref();
    const_cast<Horizon*>(horizon_)
	->objectChanged().notify( mCB(this,HorizonZTransform,horChangeCB) );

    fromzdomaininfo_.setID( horizon_->dbKey().toString() );
    tozdomaininfo_.setID( horizon_->dbKey().toString() );

    horchanged_ = true;
    calculateHorizonRange();
}


void EM::HorizonZTransform::transformTrc( const TrcKey& trckey,
	const SamplingData<float>& sd, int sz,float* res ) const
{
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = mUdf(float);

	return;
    }

    if ( !horizon_ )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = sd.atIndex( idx );

	return;
    }

    float top, bottom;
    if ( !getTopBottom( trckey, top, bottom ) )
    {
	for ( int idx=sz-1; idx>=0; idx-- )
	    res[idx] = mUdf(float);

	return;
    }

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const float depth = sd.atIndex( idx );
	if ( depth<top ) res[idx] = depth-top;
	else if ( depth>bottom ) res[idx] = depth-bottom;
	else res[idx] = 0;
    }
}


void EM::HorizonZTransform::transformTrcBack( const TrcKey& trckey,
	const SamplingData<float>& sd, int sz,float* res ) const
{
    for ( int idx=sz-1; idx>=0; idx-- )
	res[idx] = mUdf(float);

    if ( !horizon_ || mIsUdf(sd.start) || mIsUdf(sd.step) )
	return;

    float top, bottom;
    if ( !getTopBottom( trckey, top, bottom ) )
	return;

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const float depth = sd.atIndex( idx );
	if ( depth<=0 ) res[idx] = depth+top;
	else res[idx] = depth+bottom;
    }
}


Interval<float> EM::HorizonZTransform::getZInterval( bool from ) const
{
    if ( from )
	return SI().zRange();

    if ( horchanged_ )
	const_cast<HorizonZTransform*>(this)->calculateHorizonRange();

    if ( horchanged_ )
	return SI().zRange();

    Interval<float> intv( SI().zRange().start-depthrange_.stop,
			  SI().zRange().stop-depthrange_.start );
    const float step = SI().zStep();
    float idx = intv.start / step;
    intv.start = Math::Floor(idx) * step;
    idx = intv.stop / step;
    intv.stop = ceil(idx) * step;
    return intv;
}


void EM::HorizonZTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );

    if ( horizon_ )
	par.set( sKeyHorizonID(), horizon_->dbKey() );
}


bool EM::HorizonZTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    DBKey objkey;
    if ( !par.get( sKeyHorizonID(), objkey ) )
	return true;

    EM::ObjectManager& horman = EM::getMgr( objkey );
    RefMan<EM::Object> emobj = horman.getObject( objkey );
    if ( !emobj )
	return false;

    mDynamicCastGet(EM::Horizon*,hor,emobj.ptr())
    if ( !hor )
	return false;

    setHorizon( *hor );
    return true;
}


float EM::HorizonZTransform::getZIntervalCenter( bool from ) const
{
    if ( from )
	return ZAxisTransform::getZIntervalCenter( from );

    return 0;
}


void EM::HorizonZTransform::horChangeCB(CallBacker*)
{
    horchanged_ = true;

    if ( horizon_ && !horizon_->hasBurstAlert() )
	change_.trigger();
}


void EM::HorizonZTransform::calculateHorizonRange()
{
    if ( !horizon_ ) return;

    PtrMan<ObjectIterator> iterator = horizon_->createIterator();
    if ( !iterator ) return;

    bool isset = false;

    EM::PosID pid = iterator->next();
    while ( !pid.isInvalid() )
    {
	const float depth = (float) horizon_->getPos( pid ).z_;
	if ( !mIsUdf( depth ) )
	{
	    if ( isset ) depthrange_.include( depth, false );
	    else { depthrange_.start = depthrange_.stop = depth; isset=true; }
	}

	pid = iterator->next();
    }

    horchanged_ = false;
}


bool EM::HorizonZTransform::getTopBottom( const TrcKey& trckey, float& top,
				      float& bottom ) const
{
    mDynamicCastGet(const Horizon3D*,hor3d,horizon_)
    mDynamicCastGet(const Horizon2D*,hor2d,horizon_)
    TypeSet<float> depths;
    TrcKey hortrckey;
    if ( trckey.geomSystem()==horizon_->geomSystem() )
	hortrckey = trckey;
    else
    {
	const Coord crd = trckey.getCoord();
	if ( crd.isUdf() )
	    return false;

	if ( hor3d )
	    hortrckey = TrcKey( SurvGeom::get3D().transform(crd) );
	else if ( hor2d )
	{
	    float bestdist2 = mUdf(float);
	    for ( int idx=0; idx<hor2d->geometry().nrLines(); idx++ )
	    {
		const auto& geom2d = SurvGeom::get2D(
					hor2d->geometry().geomID(idx) );
		if ( geom2d.isEmpty() )
		    continue;

		const auto trcnr =
		    geom2d.tracePosition( crd, geom2d.averageTrcDist() * 3 );
		const Coord trccrd = geom2d.getCoord( trcnr );
		if ( trccrd.isDefined() )
		{
		    const float dist2 = (float)crd.sqDistTo( trccrd );
		    if ( mIsUdf(bestdist2) || dist2<bestdist2 )
		    {
			hortrckey = TrcKey( geom2d.geomID(), trcnr );
			bestdist2 = dist2;
		    }
		}
	    }
	}
    }

    EM::PosID pid = horizon_->geometry().getPosID( hortrckey );
    float depth = (float)horizon_->getPos( pid ).z_;
    if ( mIsUdf( depth ) && hor3d )
    {
	const BinID bid = hortrckey.binID();
	const Geometry::BinIDSurface* geom =
	    hor3d->geometry().geometryElement();
	depth =(float)geom->computePosition(Coord(bid.inl(),bid.crl()) ).z_;
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
