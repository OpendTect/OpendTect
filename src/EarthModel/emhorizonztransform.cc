/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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


void HorizonZTransform::transform( const BinID& bid,
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
    if ( !getTopBottom( bid, top, bottom ) )
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


void HorizonZTransform::transformBack( const BinID& bid,
	const SamplingData<float>& sd, int sz,float* res ) const
{
    for ( int idx=sz-1; idx>=0; idx-- )
	res[idx] = mUdf(float);

    if ( !horizon_ || mIsUdf(sd.start) || mIsUdf(sd.step) )
	return;

    float top, bottom;
    if ( !getTopBottom( bid, top, bottom ) )
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
    intv.start = floor(idx) * step;
    idx = intv.stop / step;
    intv.stop = ceil(idx) * step;
    return intv;
}


int HorizonZTransform::lineIndex( const char* lnm ) const
{
    mDynamicCastGet(const Horizon2D*,hor2d,horizon_)
    return hor2d ? hor2d->geometry().lineIndex( lnm ) : 0;

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

    PtrMan<EMObjectIterator> iterator = horizon_->createIterator( -1, 0 );
    if ( !iterator ) return;

    bool isset = false;

    EM::PosID pid = iterator->next();
    while ( pid.objectID()!=-1  )
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


bool HorizonZTransform::getTopBottom( const BinID& bid, float& top,
				      float& bottom ) const
{
    mDynamicCastGet(const Horizon3D*,hor3d,horizon_)
    mDynamicCastGet(const Horizon2D*,hor2d,horizon_)
    TypeSet<float> depths;
    for ( int idx=horizon_->nrSections()-1; idx>=0; idx-- )
    {
	const SectionID sid = horizon_->sectionID( idx );
	float depth = mUdf(float);
	if ( hor3d )
	{
	    const Geometry::BinIDSurface* geom =
		hor3d->geometry().sectionGeometry(sid);
	    depth = (float) geom->computePosition( Coord(bid.inl,bid.crl) ).z;
	}
	else if ( hor2d )
	    depth = (float) horizon_->getPos( sid, bid.toInt64() ).z;

	if ( !mIsUdf(depth) )
	    depths += depth;
    }

    if ( depths.size()>1 )
	sort_array( depths.arr(), depths.size() );
    else if ( !depths.size() )
	return false;

    top = depths[0];
    bottom = depths[depths.size()-1];

    return true;
}

} // namespace EM
