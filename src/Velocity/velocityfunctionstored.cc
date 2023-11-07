/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityfunctionstored.h"

#include "binidvalset.h"
#include "ioman.h"
#include "keystrs.h"
#include "survinfo.h"
#include "picksettr.h"
#include "pickset.h"
#include "sorting.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "varlenarray.h"
#include "veldesc.h"
#include "zvalseriesimpl.h"


namespace Vel
{

StoredFunction::StoredFunction( StoredFunctionSource& source )
    : Function(source)
{}


StoredFunction::~StoredFunction()
{}


ZSampling StoredFunction::getAvailableZ() const
{
    return desiredrg_;
}


bool StoredFunction::moveTo( const BinID& bid )
{
    if ( !Function::moveTo(bid) )
	return false;

    if ( getDesc().isUdf() )
	copyDescFrom( getSource() );

    mDynamicCastGet( StoredFunctionSource&, source, source_ );
    if ( !source.getVel(bid,zval_,vel_) )
	return false;

    mAllocVarLenIdxArr( int, idxs, zval_.size() );
    if ( !mIsVarLenArrOK(idxs) )
	return false;

    sort_coupled( zval_.arr(), mVarLenArr(idxs), zval_.size() );

    mAllocLargeVarLenArr( double, vels, vel_.size() );
    if ( !vels )
	return false;

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
	vels[idx] = vel_[idxs[idx]];

    for ( int idx=zval_.size()-1; idx>=0; idx-- )
	vel_[idx] = vels[idx];

    return true;
}


bool StoredFunction::computeVelocity( float z0, float dz, int sz,
				      float* res ) const
{
    if ( vel_.isEmpty() )
	return false;

    const ArrayValueSeries<double,double> vels_in( (double*)(vel_.arr()), false,
						   vel_.size() );
    const ArrayZValues<double> zvals_in( (TypeSet<double>&)(zval_),
					 source_.zDomain() );
    const SamplingData<float> sd_out( z0, dz );
    const RegularZValues zvals_out( sd_out, sz, zDomain() );
    ArrayValueSeries<double,float> vels_out( res, false, sz );
    const Worker worker( source_.getDesc(), SI().seismicReferenceDatum(),
			 UnitOfMeasure::surveyDefSRDStorageUnit() );
    if ( !worker.sampleVelocities(vels_in,zvals_in,zvals_out,vels_out) )
	return false;

    const UnitOfMeasure* funcveluom = velUnit();
    const UnitOfMeasure* funcsrcveluom = source_.velUnit();
    if ( funcveluom != funcsrcveluom )
    {
	for ( int idx=0; idx<sz; idx++ )
	    convValue( res[idx], funcsrcveluom, funcveluom );
    }

    return true;
}


// StoredFunctionSource

static const char* sKeyVelocityFunction = "Velocity Function";

const char* StoredFunctionSource::sKeyVelocityType()
{ return VelocityDesc::sKeyVelocityType(); }


IOObjContext& StoredFunctionSource::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ret, = nullptr );

    if ( !ret )
    {
	auto* newret =
		    new IOObjContext(PickSetTranslatorGroup::ioContext());
	newret->setName( "RMO picks" );
	newret->toselect_.require_.set( sKey::Type(), sKeyVelocityFunction );

	ret.setIfNull( newret, true );
    }

    return *ret;
}


StoredFunctionSource::StoredFunctionSource()
    : FunctionSource()
    , desc_(*new VelocityDesc)
    , veldata_(0,true)
{
    desc_.setUnit( UnitOfMeasure::surveyDefVelStorageUnit() );
}


StoredFunctionSource::~StoredFunctionSource()
{
    delete &desc_;
}


void StoredFunctionSource::initClass()
{ FunctionSource::factory().addCreator( create, sFactoryKeyword() ); }


void StoredFunctionSource::setData( const BinIDValueSet& bvs,
				    const VelocityDesc& vd,
				    const ZDomain::Info& zinfo )
{
    veldata_ = bvs;
    desc_ = vd;
    setZDomain( zinfo );
}


bool StoredFunctionSource::store( const MultiID& velid )
{
    PtrMan<IOObj> ioobj = IOM().get( velid );
    if ( !ioobj )
	return false;

    if ( velid != mid_ )
    {
	mid_ = velid;
	//Trigger something?
    }

    RefMan<Pick::Set> ps = new Pick::Set( ioobj->name() );
    BinIDValueSet::SPos arrpos;

    const bool is2d = veldata_.is2D();
    while ( veldata_.next(arrpos,false) )
    {
	const float* vals = veldata_.getVals(arrpos);
	const BinID bid = veldata_.getBinID(arrpos);

	Coord3 pos;
	pos.z = vals[0];
	TrcKey tk( bid );
	if ( is2d )
	    tk.setIs2D();

	pos.coord() = Survey::GM().toCoord( tk );
	const Coord3 dir( vals[1], mUdf(float), mUdf(float) );
	::Pick::Location pickloc( pos, dir );
	if ( is2d )
	    pickloc.setTrcKey( tk );

	ps->add( pickloc );
    }

    ps->setZDomain( zDomain() );
    desc_.fillPar( ps->pars_ );
    if ( !PickSetTranslator::store(*ps,ioobj,errmsg_) )
	return false;

    fillIOObjPar( ioobj->pars() );

    if ( !IOM().commitChanges(*ioobj) )
    {
	errmsg_ = mFromUiStringTodo(
		    uiStrings::phrCannotWriteDBEntry( ioobj->uiName() ));
	return false;
    }

    return true;
}


void StoredFunctionSource::fillIOObjPar( IOPar& par ) const
{
    par.setEmpty();
    par.set( sKey::Type(), sKeyVelocityFunction );
    par.set( sKeyVelocityType(), toString(desc_.type_) );
}


bool StoredFunctionSource::setFrom( const MultiID& velid )
{
    PtrMan<IOObj> ioobj = IOM().get( velid );
    if ( !ioobj )
	return false;

    RefMan<Pick::Set> ps = new Pick::Set( ioobj->name() );
    if ( !PickSetTranslator::retrieve(*ps,ioobj,false,errmsg_) )
	return false;

    if ( !desc_.usePar(ps->pars_) )
	return false;

    setZDomain( ps->zDomain() );

    const bool is2d = ps->is2D();
    veldata_.setEmpty();
    veldata_.setNrVals( 2, false );
    veldata_.setIs2D( is2d );

    float vals[2];
    for ( int idx=ps->size()-1; idx>=0; idx-- )
    {
	const ::Pick::Location& pspick = ps->get( idx );

	TrcKey tk;
	if ( is2d )
	    tk = pspick.trcKey();
	else
	{
	    tk.setGeomSystem( OD::Geom3D );
	    tk.setFrom( pspick.pos() );
	}

	vals[0] = float (pspick.pos().z);
	vals[1] = pspick.dir().radius;
	veldata_.add( tk.binID(), vals );
    }

    mid_ = velid;
    return true;
}


void StoredFunctionSource::getAvailablePositions( BinIDValueSet& binvals) const
{
    binvals.setEmpty();
    BinIDValueSet::SPos pos;
    while ( veldata_.next(pos, true) )
	binvals.add( veldata_.getBinID(pos) );
}


StoredFunction* StoredFunctionSource::createFunction( const BinID& binid )
{
    auto* res = new StoredFunction( *this );
    if ( !res->moveTo(binid) )
    {
	delete res;
	return nullptr;
    }

    return res;
}


FunctionSource* StoredFunctionSource::create( const MultiID& mid )
{
    if ( mid.groupID() !=
	    IOObjContext::getStdDirData( IOObjContext::Loc )->groupID() )
	return nullptr;

    auto* res = new StoredFunctionSource;
    if ( !res->setFrom(mid) )
    {
	FunctionSource::factory().errMsg() = res->errMsg();
	delete res;
	return nullptr;
    }

    return res;
}


bool StoredFunctionSource::getVel( const BinID& binid, TypeSet<double>& zval,
				   TypeSet<double>& vel )
{
    zval.setEmpty();
    vel.setEmpty();

    if ( !veldata_.isValid(binid) )
	return false;

    BinIDValueSet::SPos pos = veldata_.find( binid );
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

} // namespace Vel
