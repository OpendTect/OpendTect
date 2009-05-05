/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: velocityfunctiongrid.cc,v 1.4 2009-05-05 16:48:33 cvskris Exp $";

#include "velocityfunctiongrid.h"

#include "binidvalset.h"
#include "cubesampling.h"
#include "gridder2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

namespace Vel
{


GriddedFunction::GriddedFunction( GriddedSource& source )
    : Function( source )
    , gridder_( 0 )
{}

GriddedFunction::~GriddedFunction()
{
    deepUnRef( velocityfunctions_ );
    delete gridder_;
}


bool GriddedFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    return fetchSources();
}


bool GriddedFunction::fetchSources()
{
    if ( mIsUdf(bid_.inl) || mIsUdf(bid_.crl ) )
	return false;

    mDynamicCastGet( GriddedSource&, gvs, source_ );
    ObjectSet<FunctionSource>& velfuncsources = gvs.datasources_;

    ObjectSet<const Function> velfuncs;
    TypeSet<int> velfuncsource;
    TypeSet<Coord> points;
    bool perfectfound = false;

    const Coord workpos = SI().transform( bid_ );
    if ( gridder_ && !gridder_->setGridPoint( workpos ) ) 
	return false;

    for ( int idx=0; idx<velfuncsources.size() && !perfectfound; idx++ )
    {
	BinIDValueSet bids( 0, false );
	velfuncsources[idx]->getSurroundingPositions( bid_, bids );

	if ( bids.isEmpty() )
	    continue;

	BinIDValueSet::Pos pos;

	while ( bids.next( pos ) )
	{
	    const BinID curbid = bids.getBinID(pos);
	    const Coord curpos = SI().transform( curbid );
	    if ( gridder_ && !gridder_->isPointUsable(workpos,curpos) )
		continue;

	    if ( !gridder_ && curbid!=bid_ )
		continue;

	    RefMan<const Function> velfunc = getOldFunction( curbid, idx );

	    if ( !velfunc )
		velfunc = velfuncsources[idx]->getFunction( curbid );

	    if ( !velfunc )
		continue;

	    if ( curbid==bid_ )
	    {
		perfectfound = true;
		deepUnRef(velfuncs);
		velfuncsource.erase();
		points.erase();
	    }
		    
	    velfunc->ref();
	    velfuncs += velfunc;
	    points += curpos;
	    velfuncsource += idx;

	    if ( perfectfound )
		break;
	}
    }

    deepUnRef( velocityfunctions_ );
    velocityfunctions_ = velfuncs;
    sources_ = velfuncsource;

    if ( !gridder_ )
	return perfectfound;

    return points.size() && gridder_->setPoints(points) && gridder_->init();
}


bool GriddedFunction::isInfluencedBy( const BinID& bid ) const
{
    if ( !gridder_ )
	return bid_==bid;

    return gridder_->isPointUsable( SI().transform(bid_), SI().transform(bid) );
}


void GriddedFunction::setGridder( const Gridder2D& ng )
{
    if ( gridder_ && ng==*gridder_ )
	return;

    delete gridder_;
    gridder_ = ng.clone();
    removeCache();
    fetchSources();
}


const Function*
GriddedFunction::getOldFunction( const BinID& bid, int source ) 
{ 
    for ( int idx=velocityfunctions_.size()-1; idx>=0; idx-- )
    {
	if ( sources_[idx]==source && velocityfunctions_[idx]->getBinID()==bid )
	    return velocityfunctions_[idx];
    }

    return 0;
}


StepInterval<float> GriddedFunction::getAvailableZ() const
{
    return velocityfunctions_.size() ? velocityfunctions_[0]->getAvailableZ()
				     : SI().zRange(true);
}


bool GriddedFunction::computeVelocity( float z0, float dz, int nr,
				       float* res ) const
{
    TypeSet<float> values;
    for ( int idx=0; idx<nr; idx++ )
    {
	values.erase();
	const float z = z0+idx*dz;
	for ( int idy=0; idy<velocityfunctions_.size(); idy++ )
	    values += velocityfunctions_[idy]->getVelocity( z );

	if ( gridder_ )	
	{
	    gridder_->setValues( values, false );
	    res[idx] = gridder_->getValue();
	}
	else
	    res[idx] = values[0];
    }

    return true;
}


GriddedSource::GriddedSource()
    : notifier_( this )
    , gridder_( new TriangulatedNeighborhoodGridder2D )
{
    initGridder( gridder_ );
}


GriddedSource::~GriddedSource()
{
    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    datasources_[idx]->changeNotifier()->remove(
		    mCB( this, GriddedSource, sourceChangeCB ));
    }

    deepUnRef( datasources_ );
}


const VelocityDesc& GriddedSource::getDesc() const
{
    if ( datasources_.size() )
	return datasources_[0]->getDesc();

    static const VelocityDesc defdesc( VelocityDesc::Unknown );

    return defdesc;
}


void GriddedSource::initGridder( Gridder2D* gridder )
{
    const Interval<int> inlrg = SI().inlRange( true );
    const Interval<int> crlrg = SI().crlRange( true );
    Interval<float> xrg, yrg;
    Coord c = SI().transform( BinID(inlrg.start,crlrg.start) );
    xrg.start = xrg.stop = c.x;
    yrg.start = yrg.stop = c.y;

    c = SI().transform( BinID(inlrg.start,crlrg.stop) );
    xrg.include( c.x ); yrg.include( c.y );

    c = SI().transform( BinID(inlrg.stop,crlrg.start) );
    xrg.include( c.x ); yrg.include( c.y );

    c = SI().transform( BinID(inlrg.stop,crlrg.stop) );
    xrg.include( c.x ); yrg.include( c.y );

    gridder->setGridArea( xrg, yrg );
}


void GriddedSource::setGridder( Gridder2D* ng )
{
    if ( gridder_ && ng && *ng==*gridder_ )
    {
	delete ng;
	return;
    }

    delete gridder_;
    gridder_ = ng;

    initGridder( gridder_ );

    functionslock_.readLock();

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	func->setGridder( *gridder_ );
    }

    functionslock_.readUnLock();
}


const Gridder2D* GriddedSource::getGridder() const
{
    return gridder_;
}


void GriddedSource::setSource( ObjectSet<FunctionSource>& nvfs )
{
    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    datasources_[idx]->changeNotifier()->remove(
		    mCB( this, GriddedSource, sourceChangeCB ));
    }

    deepUnRef( datasources_ );
    datasources_ = nvfs;
    deepRef( datasources_ );

    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    datasources_[idx]->changeNotifier()->notify(
		    mCB( this, GriddedSource, sourceChangeCB ));
    }

    if ( datasources_.size() ) mid_ = datasources_[0]->multiID();
}


void GriddedSource::setSource( const TypeSet<MultiID>& mids )
{
    ObjectSet<FunctionSource> newsources;
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	FunctionSource* src =
	    FunctionSource::factory().create( "", mids[idx], false );
	if ( !src ) continue;
	src->ref();
	newsources += src;
    }

    GriddedSource::setSource( newsources );
}


void GriddedSource::getSources( TypeSet<MultiID>& mids ) const
{
    for ( int idx=0; idx<datasources_.size(); idx++ )
    {
	const MultiID& mid = datasources_[idx]->multiID();
	if ( mid.isEmpty() )
	    continue;

	mids += mid;
    }
}


const ObjectSet<FunctionSource>& GriddedSource::getSources() const
{ return datasources_; }


void GriddedSource::getAvailablePositions( HorSampling& hrg ) const
{
    hrg = HorSampling(true);
}


GriddedFunction* GriddedSource::createFunction()
{
    GriddedFunction* res = new GriddedFunction( *this );
    res->ref();

    if ( gridder_ ) res->setGridder( *gridder_ );
    res->unRefNoDelete();
    return res;
}


GriddedFunction* GriddedSource::createFunction( const BinID& binid )
{
    GriddedFunction* res = createFunction();
    res->ref();

    if ( !res->moveTo(binid) )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void GriddedSource::sourceChangeCB( CallBacker* cb )
{
    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    functionslock_.readLock();
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	if ( bid.inl!=-1 && bid.crl!=-1 && !func->isInfluencedBy(bid) )
	    continue;

	func->removeCache();
	func->fetchSources();
    }
    functionslock_.readUnLock();

    changebid_ = BinID(-1,-1);
    notifier_.trigger();
}


void GriddedSource::fillPar( IOPar& par ) const
{
    if ( !gridder_ )
	return;

    IOPar gridpar;
    gridder_->fillPar( gridpar );
    gridpar.set( sKey::Name, gridder_->name() );
    par.mergeComp( gridpar, sKeyGridder() );
}


bool GriddedSource::usePar( const IOPar& par )
{
    PtrMan<IOPar> gridpar = par.subselect( sKeyGridder() );
    if ( !gridpar )
	return true; //For now. Change later.

    BufferString nm;
    gridpar->get( sKey::Name, nm );
    Gridder2D* gridder = Gridder2D::factory().create( nm.buf() );
    if ( !gridder )
	return false;

    if ( !gridder->usePar( *gridpar ) )
    {
	delete gridder;
	return false;
    }

    setGridder( gridder );

    return true;
}


}; //namespace
