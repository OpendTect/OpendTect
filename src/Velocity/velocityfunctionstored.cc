
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Sep 2008
________________________________________________________________________

-*/

#include "velocityfunctionstored.h"

#include "binnedvalueset.h"
#include "keystrs.h"
#include "survinfo.h"
#include "picksettr.h"
#include "picksetmanager.h"
#include "sorting.h"
#include "varlenarray.h"
#include "velocitycalc.h"
#include "trigonometry.h"
#include "uistrings.h"

namespace Vel
{

const char* StoredFunctionSource::sKeyVelocityFunction()
{ return "Velocity Function"; }


const char* StoredFunctionSource::sKeyZIsTime()
{ return "Z Is Time"; }


const char* StoredFunctionSource::sKeyVelocityType()
{ return "Velocity Type"; }


IOObjContext& StoredFunctionSource::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ret, = 0 );

    if ( !ret )
    {
	IOObjContext* newret =
		    new IOObjContext(PickSetTranslatorGroup::ioContext());
	newret->setName( "RMO picks" );
	newret->toselect_.require_.set( sKey::Type(), sKeyVelocityFunction() );

	ret.setIfNull(newret,true);
    }

    return *ret;
}


StoredFunction:: StoredFunction( StoredFunctionSource& source )
    : Function( source )
{}


bool StoredFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    mDynamicCastGet( StoredFunctionSource&, source, source_ );
    if ( !source.getVel( bid, zval_, vel_ ) )
	return false;

    mAllocVarLenIdxArr( int, idxs, zval_.size() );
    if ( !mIsVarLenArrOK(idxs) ) return false;

    sort_coupled( zval_.arr(), mVarLenArr(idxs), zval_.size() );

    mAllocLargeVarLenArr( float, vels, vel_.size() );
    if ( !vels ) return false;

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
	vels[idx] = vel_[idxs[idx]];

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
	vel_[idx] = vels[idx];

    return true;
}


StoredFunctionSource::StoredFunctionSource()
    : zit_( SI().zIsTime() )
    , veldata_( 0, true )
{}


StoredFunctionSource::~StoredFunctionSource()
{}


bool StoredFunctionSource::zIsTime() const
{ return zit_; }


void StoredFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sFactoryKeyword() ); }


void StoredFunctionSource::setData( const BinnedValueSet& bvs,
				   const VelocityDesc& vd, bool zit )
{
    zit_ = zit;
    desc_ = vd;
    veldata_ = bvs;
}


bool StoredFunctionSource::store( const DBKey& velid )
{
    if ( velid != mid_ )
    {
	mid_ = velid;
	//Trigger something?
    }

    RefMan< ::Pick::Set > ps = new ::Pick::Set;
    BinnedValueSet::SPos arrpos;

    while ( veldata_.next(arrpos,false) )
    {
	const float* vals = veldata_.getVals(arrpos);
	const BinID bid = veldata_.getBinID(arrpos);

	const Coord3 pos( SI().transform(bid), vals[0] );
	const Sphere dir( vals[1], mUdf(float), mUdf(float) );
	ps->add( ::Pick::Location(pos,dir) );
    }

    IOPar psiop( ps->pars() );
    psiop.setYN( sKeyZIsTime(), zit_ );
    desc_.fillPar( psiop );
    ps->setPars( psiop );

    IOPar ioobjpars;
    fillIOObjPar( ioobjpars );
    SilentTaskRunnerProvider trprov;
    uiString errmsg = ::Pick::SetMGR().store( *ps, mid_, trprov, &ioobjpars );
    if ( !errmsg.isEmpty() )
	{ errmsg_ =  errmsg; return false; }

    return true;
}


void StoredFunctionSource::fillIOObjPar( IOPar& par ) const
{
    par.setEmpty();
    par.set( sKey::Type(), sKeyVelocityFunction() );
    par.set( sKeyVelocityType(), VelocityDesc::TypeDef().getKey(desc_.type_));
}


bool StoredFunctionSource::load( const DBKey& velid )
{
    uiRetVal uirv;
    ConstRefMan< ::Pick::Set > ps = ::Pick::SetMGR().fetch( velid, uirv );
    if ( !ps )
	{ errmsg_ = toUiString(uirv.getText()); return false; }

    const IOPar psiop( ps->pars() );
    if ( !psiop.getYN( sKeyZIsTime(), zit_ ) ||
	 !desc_.usePar( psiop ) )
	return false;

    veldata_.setEmpty();
    veldata_.setNrVals( 2 );

    ::Pick::SetIter psiter( *ps, true );
    while ( psiter.next() )
    {
	const ::Pick::Location& ploc = psiter.get();
	if ( ploc.hasPos() && ploc.hasDir() )
	{
	    float vals[2];
	    vals[0] = (float)ploc.pos().z_;
	    vals[1] = ploc.dir().radius_;
	    veldata_.add( ploc.binID(), vals );
	}
    }

    mid_ = velid;
    return true;
}


void StoredFunctionSource::getAvailablePositions( BinnedValueSet& binvals) const
{
    binvals.setEmpty();
    BinnedValueSet::SPos pos;
    while ( veldata_.next(pos, true) )
	binvals.add( veldata_.getBinID(pos) );
}


StoredFunction* StoredFunctionSource::createFunction( const BinID& binid )
{
    StoredFunction* res = new StoredFunction( *this );
    if ( !res->moveTo(binid) )
	{ delete res; return 0; }
    return res;
}


FunctionSource* StoredFunctionSource::create( const DBKey& mid )
{
    StoredFunctionSource* res = new StoredFunctionSource;
    if ( !res->load( mid ) )
	{ delete res; return 0; }
    return res;
}


bool StoredFunctionSource::getVel( const BinID& binid, TypeSet<float>& zval,
				   TypeSet<float>& vel )
{
    zval.erase(); vel.erase();
    if ( !veldata_.isValid(binid) )
	return false;

    BinnedValueSet::SPos pos = veldata_.find( binid );
    do
    {
	if ( veldata_.getBinID(pos)!=binid )
	    break;

	const float* vals = veldata_.getVals( pos );
	zval += vals[0];
	vel += vals[1];

    } while ( veldata_.next(pos) );

    return true;
}


bool StoredFunction::computeVelocity( float z0, float dz, int nr,
				      float* res ) const
{
    if ( vel_.isEmpty() )
	return false;

    mDynamicCastGet( StoredFunctionSource&, source, source_ );
    const SamplingData<double> sd = getDoubleSamplingData(
						SamplingData<float>(z0,dz) );
    const int zsz = zval_.size();
    mAllocVarLenArr( double, zvals, zsz );
    if ( !mIsVarLenArrOK(zvals) )
	return false;

    for ( int idx=0; idx<zsz; idx++ )
	zvals[idx] = zval_[idx];

    if ( getDesc().type_==VelocityDesc::Interval )
    {
	return sampleVint( vel_.arr(), zvals, zsz, sd, res, nr );
    }
    else if ( getDesc().type_==VelocityDesc::RMS && source.zIsTime() )
    {
	return sampleVrms( vel_.arr(), 0, 0, zvals, zsz, sd, res, nr );
    }
    else if ( getDesc().type_==VelocityDesc::Avg )
    {
	return sampleVavg( vel_.arr(), zvals, zsz, sd, res, nr );
    }
    else if ( getDesc().type_==VelocityDesc::Delta )
    {
	sampleIntvThomsenPars( vel_.arr(), zvals, zsz, sd, nr, res );
	return true;
    }
    else if ( getDesc().type_==VelocityDesc::Epsilon )
    {
	sampleIntvThomsenPars( vel_.arr(), zvals, zsz, sd, nr, res );
	return true;
    }
    else if ( getDesc().type_==VelocityDesc::Eta )
    {
	sampleEffectiveThomsenPars( vel_.arr(), zvals, zsz, sd, nr, res );
	return true;
    }

    return false;
}


StepInterval<float> StoredFunction::getAvailableZ() const
{
    return desiredrg_;
}

} // namespace Vel
