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

namespace EM
{

static const ZDomain::Def& getZDom()
{
    return SI().zIsTime() ? ZDomain::Def::get( "Time-Flattened" )
			  : ZDomain::Def::get( "Depth-Flattened" );
}

HorizonZTransform::HorizonZTransform()
    : ZAxisTransform(ZDomain::SI(),getZDom())
    , horizon_( 0 )
    , horchanged_( false )
    , change_( this )
{}


HorizonZTransform::~HorizonZTransform()
{
    if ( horizon_ )
    {
	const_cast<Horizon*>(horizon_)->change.remove(
		mCB(this,HorizonZTransform,horChangeCB) );
	horizon_->unRef();
    }
}


void HorizonZTransform::setHorizon( const Horizon& hor )
{
    if ( horizon_ )
    {
	const_cast<Horizon*>(horizon_)
	    ->change.remove( mCB(this,HorizonZTransform,horChangeCB) );
	horizon_->unRef();
    }

    horizon_ = &hor;
    horizon_->ref();
    const_cast<Horizon*>(horizon_)
	->change.notify( mCB(this,HorizonZTransform,horChangeCB) );

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


void HorizonZTransform::transformTrc( const TrcKey& trckey,
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


void HorizonZTransform::transformTrcBack( const TrcKey& trckey,
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


Interval<float> HorizonZTransform::getZInterval( bool from ) const
{
    if ( from ) return SI().zRange(true);

    if ( horchanged_ )
	const_cast<HorizonZTransform*>(this)->calculateHorizonRange();

    if ( horchanged_ ) return SI().zRange(true);

    Interval<float> intv( SI().zRange(true).start-depthrange_.stop,
			  SI().zRange(true).stop-depthrange_.start );
    const float step = SI().zRange(true).step;
    float idx = intv.start / step;
    intv.start = Math::Floor(idx) * step;
    idx = intv.stop / step;
    intv.stop = ceil(idx) * step;
    return intv;
}


void HorizonZTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );

    if ( horizon_ )
	par.set( sKeyHorizonID(), horizon_->multiID() );
}


bool HorizonZTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar( par ) )
	return false;

    MultiID mid;
    if ( !par.get( sKeyHorizonID(), mid ) )
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


float HorizonZTransform::getZIntervalCenter( bool from ) const
{
    if ( from )
	return ZAxisTransform::getZIntervalCenter( from );

    return 0;
}


void HorizonZTransform::horChangeCB(CallBacker*)
{
    horchanged_ = true;

    if ( horizon_ && !horizon_->hasBurstAlert() )
	change_.trigger();
}


void HorizonZTransform::calculateHorizonRange()
{
    if ( !horizon_ ) return;

    PtrMan<EMObjectIterator> iterator = horizon_->createIterator();
    if ( !iterator ) return;

    bool isset = false;

    EM::PosID pid = iterator->next();
    while ( pid.isValid() )
    {
	const float depth = (float) horizon_->getPos( pid ).z;
	if ( !mIsUdf( depth ) )
	{
	    if ( isset ) depthrange_.include( depth, false );
	    else { depthrange_.start = depthrange_.stop = depth; isset=true; }
	}

	pid = iterator->next();
    }

    horchanged_ = false;
}


bool HorizonZTransform::getTopBottom( const TrcKey& trckey, float& top,
				      float& bottom ) const
{
    mDynamicCastGet(const Horizon3D*,hor3d,horizon_)
    mDynamicCastGet(const Horizon2D*,hor2d,horizon_)
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
