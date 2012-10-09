/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id$";

#include "velocityfunctiongrid.h"

#include "binidvalset.h"
#include "cubesampling.h"
#include "gridder2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "task.h"

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

    //Search for perfect fit (i.e no gridding)
    if ( gvs.sourcepos_.valid(bid_) )
    {
	int funcsource;
	RefMan<const Function> velfunc = getInputFunction( bid_, funcsource );
	if  ( velfunc )
	{
	    velfunc->ref();
	    velfuncs += velfunc;
	    velfuncsource += funcsource;
	}
    }

    if ( !velfuncs.size() ) //no perfect fit
    {
	const Coord workpos = SI().transform( bid_ );
	if ( gridder_ && (!gridder_->setGridPoint( workpos ) || 
	     !gridder_->init()) )
	    return false;

	const TypeSet<BinID>& binids = gvs.gridsourcebids_;
	const TypeSet<int>& gridinput = gridder_->usedValues();
	for ( int idx=0; idx<gridinput.size(); idx++ )
	{
	    const BinID curbid = binids[gridinput[idx]];

	    int funcsource;
	    RefMan<const Function> velfunc = getInputFunction(curbid,funcsource);
	    if ( !velfunc )
	    {
		pErrMsg("Error");
		deepUnRef( velfuncs );
		return false;
	    }

	    velfunc->ref();
	    velfuncs += velfunc;
	    velfuncsource += funcsource;
	}
    }

    if ( !velfuncs.size() )
	return false;

    if ( velfuncs.size()>1 && !gridder_ )
    {
	deepUnRef( velfuncs );
	return false;
    }

    deepUnRef( velocityfunctions_ );
    velocityfunctions_ = velfuncs;
    sources_ = velfuncsource;

    if ( !gridvalues_.size()!=gvs.gridsourcecoords_.size() )
	gridvalues_.setSize( gvs.gridsourcecoords_.size(), mUdf(float) );

    return true;
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

    if ( ng.getPoints() )
    {
	//Hack to hope for a better randomization 
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( gridder_->setPoints( *ng.getPoints() ) )
		break;
	}
    }


    removeCache();
    fetchSources();
}


RefMan<const Function>
GriddedFunction::getInputFunction( const BinID& bid, int& funcsource ) 
{ 
    mDynamicCastGet( GriddedSource&, gvs, source_ );
    ObjectSet<FunctionSource>& velfuncsources = gvs.datasources_;

    RefMan<const Function> velfunc = 0;
    for ( funcsource=0; funcsource<velfuncsources.size();
	  funcsource++ )
    {
	for ( int idx=velocityfunctions_.size()-1; idx>=0; idx-- )
	{
	    if ( sources_[idx]==funcsource &&
		    velocityfunctions_[idx]->getBinID()==bid )
	    {
		velfunc = velocityfunctions_[idx];
		break;
	    }
	}

	if ( !velfunc )
	    velfunc = velfuncsources[funcsource]->getFunction( bid );

	if ( velfunc ) break;
    }

    return velfunc;
}


StepInterval<float> GriddedFunction::getAvailableZ() const
{
    return velocityfunctions_.size() ? velocityfunctions_[0]->getAvailableZ()
				     : SI().zRange(true);
}


bool GriddedFunction::computeVelocity( float z0, float dz, int nr,
				       float* res ) const
{
    mDynamicCastGet( GriddedSource&, gvs, source_ );
    const bool nogridding = velocityfunctions_.size()==1;

    const bool doinverse = getDesc().isVelocity();

    for ( int idx=0; idx<nr; idx++ )
    {
	const float z = z0+idx*dz;
	if ( nogridding )
	{
	    res[idx] = velocityfunctions_[0]->getVelocity( z );
	    continue;
	}

	TypeSet<int> undefpos;
	int nrnull = 0;

	double gridvaluesum = 0;
	int nrgridvalues = 0;

	const TypeSet<int>& usedpoints = gridder_->usedValues();
	for ( int idy=usedpoints.size()-1; idy>=0; idy-- )
	{
	    const float value = velocityfunctions_[idy]->getVelocity( z );
	    if ( doinverse && mIsZero(value,1e-3) )
	    {
		undefpos += usedpoints[idy];
		nrnull ++;
		continue;
	    }

	    if ( mIsUdf(value) )
	    {
		undefpos += usedpoints[idy];
		continue;
	    }

	    const float gridvalue = doinverse ? 1.0/value : value;

	    gridvalues_[usedpoints[idy]] = gridvalue;
	    gridvaluesum += gridvalue;
	    nrgridvalues++;
	}

	if ( nrgridvalues<usedpoints.size() )
	{
	    if ( nrnull==usedpoints.size() ) //All are null
	    {
		res[idx] = 0;
		continue;
	    }

	    if ( !nrgridvalues )
	    {
		res[idx] = mUdf(float);
		continue;
	    }

	    const float averageval = gridvaluesum/nrgridvalues;
	    for ( int idy=undefpos.size()-1; idy>=0; idy-- )
		gridvalues_[undefpos[idy]] = averageval;
	}

	gridder_->setValues( gridvalues_, false );
	const float val = gridder_->getValue();
	if ( doinverse )
	    res[idx] = mIsZero(val, 1e-7 ) ? mUdf(float) : 1.0/val;
	else
	    res[idx] = val;
    }

    return true;
}


GriddedSource::GriddedSource()
    : notifier_( this )
    , gridder_( new TriangulatedGridder2D )
    , sourcepos_( 0, false )
    , gridderinited_( false )
{ initGridder(); }


GriddedSource::~GriddedSource()
{
    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    datasources_[idx]->changeNotifier()->remove(
		    mCB( this, GriddedSource, sourceChangeCB ));
    }

    deepUnRef( datasources_ );
    delete gridder_;
}


const VelocityDesc& GriddedSource::getDesc() const
{
    if ( datasources_.size() )
	return datasources_[0]->getDesc();

    static const VelocityDesc defdesc( VelocityDesc::Unknown );

    return defdesc;
}


class GridderSourceFilter : public ParallelTask
{
public:
GridderSourceFilter( const BinIDValueSet& bvs, TypeSet<BinID>& bids,
		     TypeSet<Coord>& coords )
    : bvs_( bvs )
    , bids_( bids )
    , coords_( coords )
    , moretodo_( true )
{}


od_int64 nrIterations() const { return bvs_.totalSize(); }
bool doWork(od_int64 start, od_int64 stop, int )
{
    TypeSet<BinID> sourcebids;
    const BinID step( SI().inlRange(false).step, SI().crlRange(false).step );
    while ( true )
    {
	TypeSet<BinIDValueSet::Pos> positions;
	lock_.lock();
	if ( moretodo_ )
	{
	    for ( int idx=0; idx<10000; idx++ )
	    {
		if ( !bvs_.next(pos_) )
		{
		    moretodo_ = false;
		    break;
		}

		positions += pos_;
	    }
	}

	lock_.unLock();
	if ( !positions.size() )
	    break;

	for ( int idx=positions.size()-1; idx>=0; idx-- )
	{
	    const BinID bid = bvs_.getBinID(positions[idx]);
	    BinID neighbor( bid.inl-step.inl, bid.crl-step.crl );
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl += step.crl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl += step.crl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }

	    neighbor.crl = bid.crl-step.crl;
	    neighbor.inl += step.inl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl += 2*step.crl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }

	    neighbor.crl = bid.crl-step.crl;
	    neighbor.inl += step.inl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl += step.crl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl += step.crl;
	    if ( !bvs_.valid(neighbor) ) { sourcebids+=bid; continue; }
	}
    }

    TypeSet<Coord> coords;

    for ( int idx=0; idx<sourcebids.size(); idx++ )
	coords += SI().transform( sourcebids[idx] );

    Threads::MutexLocker lock( lock_ );
    bids_.append( sourcebids );
    coords_.append( coords );

    return true;
}
protected:

    const BinIDValueSet& bvs_;
    BinIDValueSet::Pos  pos_;

    TypeSet<BinID>&     bids_;
    TypeSet<Coord>&     coords_;
    bool                moretodo_;

    Threads::Mutex      lock_;
};


bool GriddedSource::initGridder()
{
    if ( gridderinited_ )
	return true;

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

    gridder_->setGridArea( xrg, yrg );

    sourcepos_.empty();
    gridsourcecoords_.erase();
    gridsourcebids_.erase();


    for ( int idx=0; idx<datasources_.size(); idx++ )
    {
	BinIDValueSet bids( 0, false );
	datasources_[idx]->getAvailablePositions( bids );

	sourcepos_.append( bids );
    }

    GridderSourceFilter filter( sourcepos_, gridsourcebids_,gridsourcecoords_);
    if ( !filter.execute( true ) || !gridder_->setPoints( gridsourcecoords_ ) )
	return false;

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	if ( func->getGridder() )
	{
	    //Hack to hope for a better randomization 
            for ( int idy=0; idy<3; idy++ )
	    {
		if ( func->getGridder()->setPoints( gridsourcecoords_ ) )
		    break;
	    }
	}
    }


    gridderinited_ = true;
    return true;
}


void GriddedSource::setGridder( Gridder2D* ng )
{
    if ( gridderinited_ && gridder_ && ng && *ng==*gridder_ )
    {
	delete ng;
	return;
    }

    gridderinited_ = false;

    delete gridder_;
    gridder_ = ng;

    initGridder();

    Threads::MutexLocker lock( lock_ );
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	func->setGridder( *gridder_ );
    }
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

    gridderinited_ = false;
    initGridder();
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


GriddedFunction* GriddedSource::createFunction()
{
    GriddedFunction* res = new GriddedFunction( *this );
    if ( gridder_ ) res->setGridder( *gridder_ );
    return res;
}


GriddedFunction* GriddedSource::createFunction( const BinID& binid )
{
    GriddedFunction* res = createFunction();
    if ( !res->moveTo(binid) )
    {
	delete res;
	return 0;
    }
    
    return res;
}


void GriddedSource::sourceChangeCB( CallBacker* cb )
{
    gridderinited_ = false;
    initGridder();

    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    Threads::MutexLocker lock( lock_ );
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	if ( bid.inl!=-1 && bid.crl!=-1 && !func->isInfluencedBy(bid) )
	    continue;

	func->removeCache();
	func->fetchSources();
    }

    lock.unLock();

    changebid_ = BinID(-1,-1);
    notifier_.trigger();
}


void GriddedSource::fillPar( IOPar& par ) const
{
    if ( !gridder_ )
	return;

    IOPar gridpar;
    gridder_->fillPar( gridpar );
    gridpar.set( sKey::Name, gridder_->factoryKeyword() );
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
