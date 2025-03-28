/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunctiongrid.h"

#include "binidvalset.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "iopar.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "veldesc.h"


namespace Vel
{

GriddedFunction::GriddedFunction( GriddedSource& source )
    : Function(source)
{}


GriddedFunction::~GriddedFunction()
{
    deepUnRef( velocityfunctions_ );
    delete gridder_;
    unRefPtr( directsource_ );
}


bool GriddedFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo(bid) )
	return false;

    fetchPerfectFit( bid );
    if ( !gridder_ )
	return false;

    if ( gridder_->allPointsAreRelevant() )
	return true;

    return fetchSources();
}


void GriddedFunction::fetchPerfectFit( const BinID& bid )
{
    mDynamicCastGet( GriddedSource&, gvs, source_ );
    if ( !gvs.sourcepos_.isValid(bid) )
    {
	unRefAndNullPtr( directsource_ );
	return;
    }

    int funcsource;
    ConstRefMan<Function> velfunc = getInputFunction( bid_, funcsource );
    if ( !velfunc )
    {
	unRefAndNullPtr( directsource_ );
	return;
    }

    velfunc->ref();
    directsource_ = velfunc.ptr();
}


#define mVelFuncCreator() \
    ConstRefMan<Function> velfunc = getInputFunction(curbid,funcsource); \
    if ( !velfunc ) \
    { \
	pErrMsg("Error"); \
	deepUnRef( velfuncs ); \
	return false; \
    } \
    velfunc->ref(); \
    velfuncs += velfunc.ptr(); \
    velfuncsource += funcsource; \


bool GriddedFunction::fetchSources()
{
    ObjectSet<const Function> velfuncs;
    TypeSet<int> velfuncsource;

    const TypeSet<Coord>* gridderpoints = gridder_->getPoints();

    TypeSet<double> weights;
    TypeSet<int> usedpoints;
    if ( gridder_->allPointsAreRelevant() && gridderpoints )
    {
	const TypeSet<Coord>::size_type nrpoints = gridderpoints->size();
	for ( TypeSet<Coord>::size_type idx=0; idx<nrpoints; idx++ )
	    usedpoints += idx;
    }
    else
    {
	if ( mIsUdf(bid_.inl()) || mIsUdf(bid_.crl() ) )
	    return false;

	const Coord workpos = SI().transform( bid_ );
	gridder_->getWeights(workpos,weights,usedpoints);
    }


    mDynamicCastGet( GriddedSource&, gvs, source_ );
    const TypeSet<BinID>& binids = gvs.gridsourcebids_;

    if ( binids.isEmpty() ) return false;
    int funcsource;

    for ( TypeSet<Coord>::size_type idx=0; idx<usedpoints.size(); idx++ )
    {
	const BinID curbid = binids[usedpoints[idx]];
	mVelFuncCreator();
    }

    if ( usedpoints.isEmpty() )
    {
	const BinID curbid = binids[0];
	mVelFuncCreator();
    }

    if ( velfuncs.isEmpty() )
	return false;

    if ( velfuncs.size()>1 && !gridder_ )
    {
	deepUnRef( velfuncs );
	return false;
    }

    deepUnRef( velocityfunctions_ );
    velocityfunctions_ = velfuncs;
    sources_ = velfuncsource;

    if ( gridvalues_.size()!=gvs.gridsourcecoords_.size() )
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
	    if ( gridder_->setPoints(*ng.getPoints()) )
		break;
	}
    }

    removeCache();
    fetchSources();
}


void GriddedFunction::setLayerModel( const InterpolationLayerModel* mdl )
{ layermodel_ = mdl; }


ConstRefMan<Function>
GriddedFunction::getInputFunction( const BinID& bid, int& funcsource )
{
    mDynamicCastGet( GriddedSource&, gvs, source_ );
    ObjectSet<FunctionSource>& velfuncsources = gvs.datasources_;

    ConstRefMan<Function> velfunc;
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

	if ( velfunc )
	    break;
    }

    return velfunc;
}


ZSampling GriddedFunction::getAvailableZ() const
{
    return velocityfunctions_.isEmpty() ? SI().zRange(true)
				: velocityfunctions_.first()->getAvailableZ();
}


bool GriddedFunction::computeVelocity( float z0, float dz, int nr,
				       float* res ) const
{
    const bool nogridding = directsource_ || (velocityfunctions_.size() == 1);
    const bool doinverse = nogridding ? false : getDesc().isVelocity();
    Coord workpos = Coord::udf();
    if ( !nogridding )
    {
	const auto* geom = Survey::GM().getGeometry( geomid_ );
	if ( geom )
	    workpos = geom->toCoord( bid_ );
    }

    TypeSet<double> weights;
    TypeSet<int> usedpoints;
    if ( !nogridding )
    {
	if ( !gridder_ || !gridder_->getPoints() )
	    return false;

	const TypeSet<Coord>& gridderpoints = *gridder_->getPoints();
	if ( gridder_->areWeightsValuesDependent() )
	{
	    const TypeSet<Coord>::size_type nrpoints = gridderpoints.size();
	    for ( TypeSet<Coord>::size_type idx=0; idx<nrpoints; idx++ )
		usedpoints += idx;
	}
	else if ( !gridder_->getWeights(workpos,weights,usedpoints) )
	    return false;
    }

    mDynamicCastGet(RadialBasisFunctionGridder2D*,rbfgridder,gridder_)
    const TrcKey tk( bid_ ); //TODO: Get a OD::GeomSystem from TrcKeySampling
    const Vel::Function* velsrc = directsource_ ? directsource_ :
	    !velocityfunctions_.isEmpty() ? velocityfunctions_[0] : nullptr;
    for ( int idx=0; idx<nr; idx++ )
    {
	const float z = z0+idx*dz;
	if ( velsrc && nogridding )
	{
	    res[idx] = velsrc->getVelocity( z );
	    continue;
	}

	const float layeridx = layermodel_ ?
		    layermodel_->getLayerIndex( tk, z ) : mUdf(float);
	const bool validlayer = !mIsUdf(layeridx);
	if ( !validlayer && !velsrc )
	{
	    res[idx] = mUdf(float);
	    continue;
	}

	TypeSet<int> undefpos;
	int nrnull = 0;

	double gridvaluesum = 0;
	int nrgridvalues = 0;

	for ( int idy=usedpoints.size()-1; idy>=0; idy-- )
	{
	    const Function* func = velocityfunctions_[idy];
	    const TrcKey tkfunc( func->getBinID() );
	    const float zval = layermodel_ && validlayer ?
			  layermodel_->getInterpolatedZ( tkfunc, layeridx ) : z;
	    const float value = func->getVelocity( zval );
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

	    const float gridvalue = doinverse ? 1.0f/value : value;

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

	    const float averageval = (float) (gridvaluesum/nrgridvalues);
	    for ( int idy=undefpos.size()-1; idy>=0; idy-- )
		gridvalues_[undefpos[idy]] = averageval;
	}

	gridder_->setValues( gridvalues_ );
	if ( rbfgridder )
	    gridder_->setTrend( PolyTrend::Order0 );

	const float val = rbfgridder
			? gridder_->getValue( workpos )
			: gridder_->getValue( workpos, &weights, &usedpoints );
	if ( doinverse )
	    res[idx] = mIsZero(val, 1e-7 ) ? mUdf(float) : 1.0f/val;
	else
	    res[idx] = val;
    }

    return true;
}


// GriddedSource

GriddedSource::GriddedSource( const Pos::GeomID& geomid )
    : notifier_(this)
    , gridder_(new TriangulatedGridder2D)
    , geomid_(geomid)
    , sourcepos_(0,false)
{
    sourcepos_.setIs2D( geomid.is2D() );
    initGridder();
}


GriddedSource::~GriddedSource()
{
    detachAllNotifiers();
    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    mDetachCB( *datasources_[idx]->changeNotifier(),
		       GriddedSource::sourceChangeCB );
    }

    deepUnRef( datasources_ );
    delete gridder_;
}


const VelocityDesc& GriddedSource::getDesc() const
{
    if ( !datasources_.isEmpty() )
	return datasources_.first()->getDesc();

    static const VelocityDesc defdesc( OD::VelocityType::Unknown );
    return defdesc;
}


const ZDomain::Info& GriddedSource::zDomain() const
{
    return datasources_.isEmpty() ? SI().zDomainInfo()
				  : datasources_.first()->zDomain();
}


FunctionSource& GriddedSource::setZDomain( const ZDomain::Info& )
{
    return *this;
}



class GridderSourceFilter : public ParallelTask
{
public:
GridderSourceFilter( const BinIDValueSet& bvs, TypeSet<BinID>& bids,
			TypeSet<Coord>& coords, const Pos::GeomID& geomid )
    : bvs_( bvs )
    , bids_( bids )
    , coords_( coords )
    , moretodo_( true )
    , geomid_(geomid)
{}


od_int64 nrIterations() const override { return bvs_.totalSize(); }
bool doWork(od_int64 start, od_int64 stop, int ) override
{
    TypeSet<BinID> sourcebids;

    const auto* geom = Survey::GM().getGeometry( geomid_ );
    const TrcKeyZSampling& tkzs = geom->sampling();
    const BinID step = tkzs.hsamp_.step_;

    while ( true )
    {
	TypeSet<BinIDValueSet::SPos> positions;
	Threads::Locker lckr( lock_ );
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
	lckr.unlockNow();

	if ( positions.isEmpty() )
	    break;

	for ( int idx=positions.size()-1; idx>=0; idx-- )
	{
	    const BinID bid = bvs_.getBinID( positions[idx] );
	    if ( geomid_.is2D() && bid.lineNr()!=geomid_.asInt() )
		continue;

	    BinID neighbor( bid.inl()-step.inl(), bid.crl()-step.crl() );
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl() += step.crl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl() += step.crl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }

	    neighbor.crl() = bid.crl()-step.crl();
	    neighbor.inl() += step.inl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl() += 2*step.crl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }

	    neighbor.crl() = bid.crl()-step.crl();
	    neighbor.inl() += step.inl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl() += step.crl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	    neighbor.crl() += step.crl();
	    if ( !bvs_.isValid(neighbor) ) { sourcebids+=bid; continue; }
	}
    }

    TypeSet<Coord> coords;

    for ( int idx=0; idx<sourcebids.size(); idx++ )
	coords += geom->toCoord( sourcebids[idx] );

    Threads::Locker lckr2( lock_ );
    bids_.append( sourcebids );
    coords_.append( coords );

    return true;
}
protected:

    const BinIDValueSet&	bvs_;
    BinIDValueSet::SPos		pos_;

    TypeSet<BinID>&		bids_;
    TypeSet<Coord>&		coords_;
    bool			moretodo_;
    const Pos::GeomID		geomid_;

    Threads::Lock		lock_;
};


bool GriddedSource::initGridder()
{
    if ( gridderinited_ )
	return true;

    ::Interval<float> xrg, yrg;
    if ( geomid_.is2D() )
    {
	auto* geom = Survey::GM().getGeometry( geomid_ );
	auto* geom2d = geom ? geom->as2D() : nullptr;
	if ( geom2d )
	{
	    xrg.setFrom( geom2d->data().xRange() );
	    yrg.setFrom( geom2d->data().yRange() );
	}
    }
    else
    {
	const Coord cmin = SI().minCoord( true );
	const Coord cmax = SI().maxCoord( true );
        xrg.set( cmin.x_, cmax.x_ );
        yrg.set( cmin.y_, cmax.y_ );
    }

    gridder_->setGridArea( xrg, yrg );

    sourcepos_.setEmpty();
    gridsourcecoords_.erase();
    gridsourcebids_.erase();

    for ( int idx=0; idx<datasources_.size(); idx++ )
    {
	BinIDValueSet bids( 0, false );
	bids.setIs2D( geomid_.is2D() );
	datasources_[idx]->getAvailablePositions( bids );
	sourcepos_.append( bids );
    }

    GridderSourceFilter filter( sourcepos_, gridsourcebids_, gridsourcecoords_,
			        geomid_ );
    if ( !filter.execute() ||
	 !gridder_->setPoints( gridsourcecoords_ ) )
	return false;

    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	if ( func->getGridder() )
	{
	    //Hack to hope for a better randomization
            for ( int idy=0; idy<3; idy++ )
	    {
		if ( func->getGridder()->setPoints(gridsourcecoords_) )
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

    Threads::Locker lckr( lock_ );
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
	    mDetachCB( *datasources_[idx]->changeNotifier(),
		       GriddedSource::sourceChangeCB );
    }

    deepUnRef( datasources_ );
    datasources_ = nvfs;
    deepRef( datasources_ );

    for ( int idx=datasources_.size()-1; idx>=0; idx-- )
    {
	if ( datasources_[idx]->changeNotifier() )
	    mAttachCB( *datasources_[idx]->changeNotifier(),
		       GriddedSource::sourceChangeCB );
    }

    if ( datasources_.size() )
	mid_ = datasources_[0]->multiID();

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
	if ( mid.isUdf() )
	    continue;

	mids += mid;
    }
}


const ObjectSet<FunctionSource>& GriddedSource::getSources() const
{
    return datasources_;
}


void GriddedSource::setLayerModel( const InterpolationLayerModel* mdl )
{
    layermodel_ = mdl;
}


GriddedFunction* GriddedSource::createFunction()
{
    auto* res = new GriddedFunction( *this );
    if ( gridder_ )
	res->setGridder( *gridder_ );
    if ( layermodel_ )
	res->setLayerModel( layermodel_ );
    return res;
}


GriddedFunction* GriddedSource::createFunction( const BinID& binid )
{
    auto* res = createFunction();
    if ( !res->moveTo(binid) )
    {
	delete res;
	return nullptr;
    }

    return res;
}


void GriddedSource::sourceChangeCB( CallBacker* cb )
{
    gridderinited_ = false;
    initGridder();

    mDynamicCastGet( FunctionSource*, src, cb );
    const BinID bid = src->changeBinID();

    Threads::Locker lckr( lock_ );
    for ( int idx=functions_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( GriddedFunction*, func, functions_[idx] );
	if ( !bid.isUdf() && !func->isInfluencedBy(bid) )
	    continue;

	func->removeCache();
	func->fetchSources();
    }
    lckr.unlockNow();

    changebid_ = BinID::udf();
    notifier_.trigger();
}


void GriddedSource::fillPar( IOPar& par ) const
{
    if ( !gridder_ )
	return;

    IOPar gridpar;
    gridder_->fillPar( gridpar );
    gridpar.set( sKey::Name(), gridder_->factoryKeyword() );
    par.mergeComp( gridpar, sKeyGridder() );
}


bool GriddedSource::usePar( const IOPar& par )
{
    PtrMan<IOPar> gridpar = par.subselect( sKeyGridder() );
    if ( !gridpar )
	return true; //For now. Change later.

    BufferString nm;
    gridpar->get( sKey::Name(), nm );
    Gridder2D* gridder = Gridder2D::factory().create( nm.buf() );
    if ( !gridder )
	return false;

    if ( !gridder->usePar(*gridpar) )
    {
	delete gridder;
	return false;
    }

    setGridder( gridder );

    return true;
}

} // namespace Vel
