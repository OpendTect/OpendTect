
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: velocityfunctionstored.cc,v 1.4 2009-05-05 16:48:33 cvskris Exp $";

#include "velocityfunctionstored.h"

#include "binidvalset.h"
#include "ioman.h"
#include "keystrs.h"
#include "survinfo.h"
#include "picksettr.h"
#include "pickset.h"
#include "sorting.h"
#include "varlenarray.h"
#include "velocitycalc.h"

namespace Vel
{


const char* StoredFunctionSource::sKeyVelocityFunction()
{ return "Velocity Function"; }


const char* StoredFunctionSource::sKeyHasAnisotropy()
{ return "Has Anisotropy"; }


const char* StoredFunctionSource::sKeyZIsTime()
{ return "Z Is Time"; }


const char* StoredFunctionSource::sKeyVelocityType()
{ return "Velocity Type"; }


IOObjContext& StoredFunctionSource::ioContext()
{
    static PtrMan<IOObjContext> ptr = 0;

    if ( !ptr )
    {
	ptr = new IOObjContext(PickSetTranslatorGroup::ioContext());
	ptr->setName( "RMO picks" );

	ptr->parconstraints.set( sKey::Type, sKeyVelocityFunction() );
	ptr->includeconstraints = true;
	ptr->allowcnstrsabsent = false;
    }
					    
    return *ptr;
}


StoredFunction:: StoredFunction( StoredFunctionSource& source )
    : Function( source )
{}


bool StoredFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo( bid ) )
	return false;

    mDynamicCastGet( StoredFunctionSource&, source, source_ );
    if ( !source.getVel( bid, zval_, vel_, anisotropy_ ) )
	return false;

    mAllocVarLenArr( int, idxs, zval_.size() );
    for ( int idx=zval_.size()-1; idx>=0; idx-- )
	idxs[idx] = idx;

    sort_coupled( zval_.arr(), mVarLenArr(idxs), zval_.size() );
    mAllocVarLenArr( float, vels, vel_.size() );
    mAllocVarLenArr( float, anisotropy, anisotropy_.size() );
    const bool hasanisotropy = anisotropy_.size();

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
    {
	vels[idx] = vel_[idxs[idx]];
	if ( hasanisotropy )
	    anisotropy[idx] = anisotropy_[idxs[idx]];
    }

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
    {
	vel_[idx] = vels[idx];
	if ( hasanisotropy )
	    anisotropy_[idx] = anisotropy[idx];
    }

    return true;
}


StoredFunctionSource::StoredFunctionSource()
    : zit_( SI().zIsTime() )
    , veldata_( 0, true )
    , hasanisotropy_( false )
{}


StoredFunctionSource::~StoredFunctionSource()
{}


bool StoredFunctionSource::zIsTime() const
{ return zit_; }


void StoredFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sType() ); }    


void StoredFunctionSource::setData( const BinIDValueSet& bvs,
				   const VelocityDesc& vd, bool zit )
{
    zit_ = zit;
    desc_ = vd;
    veldata_ = bvs;
}


bool StoredFunctionSource::store( const MultiID& velid )
{
    PtrMan<IOObj> ioobj = IOM().get( velid );
    if ( !ioobj )
	return false;

    if ( velid!=mid_ )
    {
	mid_ = velid;
	//Trigger something?
    }

    ::Pick::Set ps( ioobj->name() );
    BinIDValueSet::Pos arrpos;

    while ( veldata_.next(arrpos,false) )
    {
	const float* vals = veldata_.getVals(arrpos);
	const BinID bid = veldata_.getBinID(arrpos);

	const Coord3 pos( SI().transform(bid), vals[0] );
	const Coord3 dir( vals[1],
		hasanisotropy_ ? vals[2] : mUdf(float), mUdf(float) );
	::Pick::Location pickloc( pos, dir );

	ps += pickloc;
    }
    
    ps.pars_.setYN( sKeyHasAnisotropy(), hasanisotropy_ );
    ps.pars_.setYN( sKeyZIsTime(), zit_ );
    desc_.fillPar( ps.pars_ );

    if ( !PickSetTranslator::store( ps, ioobj, errmsg_ ) )
	return false;

    fillIOObjPar( ioobj->pars() );

    if ( !IOM().commitChanges(*ioobj) )
	return false;

    return true;
}


void StoredFunctionSource::fillIOObjPar( IOPar& par ) const
{
    par.clear();
    par.set( sKey::Type, sKeyVelocityFunction() );
    par.set( sKeyVelocityType(), VelocityDesc::TypeNames()[(int)desc_.type_] );
}


bool StoredFunctionSource::load( const MultiID& velid )
{
    PtrMan<IOObj> ioobj = IOM().get( velid );
    if ( !ioobj )
	return false;

    ::Pick::Set pickset( ioobj->name() );
    if ( !PickSetTranslator::retrieve( pickset, ioobj, false, errmsg_ ) )
	return false;

    if ( !pickset.pars_.getYN( sKeyHasAnisotropy(), hasanisotropy_ ) ||
	 !pickset.pars_.getYN( sKeyZIsTime(), zit_ ) ||
	 !desc_.usePar( pickset.pars_ ) )
	return false;

    veldata_.empty();
    veldata_.setNrVals( hasanisotropy_ ? 2 : 3, false );
    float vals[3];
    
    for ( int idx=pickset.size()-1; idx>=0; idx-- )
    {
	const ::Pick::Location& pspick = pickset[idx];
	const BinID bid = SI().transform( pspick.pos );

	vals[0] = pspick.pos.z;
	vals[1] = pspick.dir.radius;
	vals[2] = pspick.dir.theta;

	veldata_.add( bid, vals );
    }

    mid_ = velid;

    return true;
}


void StoredFunctionSource::getAvailablePositions( BinIDValueSet& binvals) const
{
    binvals.empty();
    BinIDValueSet::Pos pos;
    while ( veldata_.next(pos, true) )
	binvals.add( veldata_.getBinID(pos) );
}


StoredFunction* StoredFunctionSource::createFunction(const BinID& binid)
{
    StoredFunction* res = new StoredFunction( *this );
    if ( !res->moveTo(binid) )
    {
	delete res;
	return 0;
    }

    return res;
}


FunctionSource* StoredFunctionSource::create(const MultiID& mid)
{
    StoredFunctionSource* res = new StoredFunctionSource;
    if ( !res->load( mid ) )
    {
	delete res;
	return 0;
    }

    return res;
}


bool StoredFunctionSource::getVel( const BinID& binid, TypeSet<float>& zval,
				  TypeSet<float>& vel,
       				  TypeSet<float>& anisotropy )
{
    zval.erase();
    vel.erase();
    anisotropy.erase();

    if ( !veldata_.valid(binid) )
	return false;

    BinIDValueSet::Pos pos = veldata_.findFirst( binid );
    do 
    {
	if ( veldata_.getBinID(pos)!=binid )
	    break;

	const float* vals = veldata_.getVals( pos );
	zval += vals[0];
	vel += vals[1];
	if ( hasanisotropy_ )
	    anisotropy += vals[2];
    } while ( veldata_.next(pos) );

    return true;
}


bool StoredFunction::computeVelocity( float z0, float dz, int nr,
       				      float* res ) const
{
    if ( vel_.isEmpty() )
	return false;

    mDynamicCastGet( StoredFunctionSource&, source, source_ );
    if ( getDesc().type_==VelocityDesc::Interval )
    {
	return sampleVint( vel_.arr(), zval_.arr(), zval_.size(),
		    SamplingData<double>( z0, dz ), res, nr );
    }
    else if ( getDesc().type_==VelocityDesc::RMS && source.zIsTime() )
    {
	return sampleVrms( vel_.arr(), 0, 0, zval_.arr(), zval_.size(),
		SamplingData<double>( z0, dz ), res, nr );
    }

    return false;
}


StepInterval<float> StoredFunction::getAvailableZ() const
{
    return desiredrg_; 
}

}; //namespace
