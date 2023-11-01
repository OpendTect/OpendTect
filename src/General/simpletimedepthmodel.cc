/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "simpletimedepthmodel.h"

#include "ascstream.h"
#include "binidvalue.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "mnemonics.h"
#include "od_stream.h"
#include "unitofmeasure.h"
#include "zvalseriesimpl.h"


mDefSimpleTranslators(SimpleTimeDepthModel,"Simple Time-Depth Model",od,Mdl)

SimpleTimeDepthModel::SimpleTimeDepthModel()
    : TimeDepthModel()
{}


SimpleTimeDepthModel::SimpleTimeDepthModel( const MultiID& id )
    : TimeDepthModel()
{
    if ( id.isUdf() )
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( ioobj )
	readFromFile( ioobj->fullUserExpr(true) );
}


bool SimpleTimeDepthModel::isOK() const
{
    return rawtimes_.size() > 1 && rawtimes_.size() == rawdepths_.size();
}


void SimpleTimeDepthModel::setRawData( const TypeSet<float>& times,
				       const TypeSet<float>& depths )
{
    rawtimes_ = times;
    rawdepths_ = depths;
    setModel( rawdepths_.arr(), rawtimes_.arr(), rawtimes_.size() );
}


bool SimpleTimeDepthModel::readFromFile( const char* fnm )
{
    TypeSet<float> tarr, darr;

    od_istream strm( fnm );
    ascistream astrm( strm );
    astrm.next();
    IOPar pars( astrm );
    int sz = 0;
    if ( !pars.get(sKey::NrValues(),sz) || sz < 2 )
	return false;

    const UnitOfMeasure* tuom = getTimeUnit();
    const UnitOfMeasure* zuom = getDepthUnit();
    BufferString zunit;
    const UnitOfMeasure* retuom;
    const UnitOfMeasure* asctuom = tuom;
    if ( pars.get(sKey::TimeUnit(),zunit) && !zunit.isEmpty() )
    {
	retuom =  UoMR().get( zunit );
	if ( retuom && retuom->isCompatibleWith(*tuom) )
	    asctuom = retuom;
    }

    zunit.setEmpty();
    const UnitOfMeasure* asczuom = zuom;
    if ( pars.get(sKey::DepthUnit(),zunit) && !zunit.isEmpty() )
    {
	retuom =  UoMR().get( zunit );
	if ( retuom && retuom->isCompatibleWith(*zuom) )
	    asczuom = retuom;
    }

    float tval, dval;
    for ( int idx=0; idx<sz; idx++ )
    {
	strm >> tval >> dval;
	if( !strm.isOK() )
	    break;

	if ( mIsUdf(tval) || mIsUdf(dval) )
	    continue;

	tarr += getConvertedValue( tval, asctuom, tuom );
	darr += getConvertedValue( dval, asczuom, zuom );
    }

    setRawData( tarr, darr );
    return isOK();
}


const UnitOfMeasure* SimpleTimeDepthModel::getTimeUnit()
{
    return UnitOfMeasure::surveyDefTimeStorageUnit();
}


const UnitOfMeasure* SimpleTimeDepthModel::getDepthUnit()
{
    return UnitOfMeasure::surveyDefDepthUnit();
}


bool SimpleTimeDepthModel::save( const MultiID& id ) const
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    return ioobj && writeToFile( ioobj->fullUserExpr(true) );
}


bool SimpleTimeDepthModel::writeToFile( const char* fnm ) const
{
    if ( !isOK() )
	return false;

    od_ostream strm( fnm );
    IOPar pars;
    pars.set( sKey::TimeUnit(), getTimeUnit()->getLabel() );
    pars.set( sKey::DepthUnit(), getDepthUnit()->getLabel() );
    pars.set( sKey::NrValues(), rawtimes_.size() );
    pars.write( strm, "Simple Time-Depth Model" );
    for ( int idx=0; idx<rawtimes_.size(); idx++ )
	strm << rawtimes_[idx] << "\t" << rawdepths_[idx] << od_endl;

    return true;
}


// SimpleTimeDepthTransform

SimpleTimeDepthTransform::SimpleTimeDepthTransform( const ZDomain::Def& from,
						    const ZDomain::Def& to )
    : ZAxisTransform(from,to)
{
}


SimpleTimeDepthTransform::SimpleTimeDepthTransform( const ZDomain::Def& from,
						    const ZDomain::Def& to,
						    const MultiID& id )
    : ZAxisTransform(from,to)
{
    setID( id );
}


SimpleTimeDepthTransform::~SimpleTimeDepthTransform()
{
    delete tdmodel_;
}


bool SimpleTimeDepthTransform::isOK() const
{
    if ( !tdmodel_ || !tdmodel_->isOK() )
	return false;

    return ZAxisTransform::isOK();
}


void SimpleTimeDepthTransform::doTransform( const SamplingData<float>&,
				    int, float*, bool ) const
{}


void SimpleTimeDepthTransform::doTransform( const SamplingData<float>& sd,
					    const ZDomain::Info& sdzinfo,
					    int sz, float* res ) const
{
    if ( sd.isUdf() )
    {
	OD::sysMemValueSet( res, mUdf(float), sz );
	return;
    }

    const RegularZValues zvals( sd, sz, sdzinfo );
    if ( zvals.isTime() )
	for ( od_int64 idx=0; idx<sz; idx++ )
	    res[idx] = tdmodel_->getDepth( float (zvals[idx]) );
    else
	for ( od_int64 idx=0; idx<sz; idx++ )
	    res[idx] = tdmodel_->getTime( float (zvals[idx]) );
}


bool SimpleTimeDepthTransform::setID( const MultiID& mid )
{
    tozdomaininfo_.pars_.set( sKey::ID(), mid );
    delete tdmodel_;
    tdmodel_ = new SimpleTimeDepthModel( mid );
    return isOK();
}


void SimpleTimeDepthTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    par.merge( tozdomaininfo_.pars_ );
}


bool SimpleTimeDepthTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    if ( !tozdomaininfo_.hasID() )
    {
	errmsg_ = tr("Z Transform: No ID for Time-Depth Model provided");
	return false;
    }

    if ( !setID(tozdomaininfo_.getID()) )
	return false;

    return true;
}


ZSampling SimpleTimeDepthTransform::getZInterval_( const ZSampling& zsamp,
						   const ZDomain::Info& from,
						   const ZDomain::Info& to,
						   bool makenice ) const
{
    ZSampling ret = getWorkZSampling( zsamp, from, to );
    if ( makenice && from != to )
    {
	const int userfac = to.def_.userFactor();
	float zstep = ret.step;
	zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
	zstep /= userfac;
	ret.step = zstep;

	const Interval<float>& rg = ret;
	const int startidx = rg.indexOnOrAfter( rg.start, zstep );
	ret.start = zstep * mNINT32( rg.atIndex( startidx, zstep ) / zstep );
	const int stopidx = rg.indexOnOrAfter( rg.stop, zstep );
	ret.stop = zstep * mNINT32( rg.atIndex( stopidx, zstep ) / zstep );
    }

    return ret;
}


ZSampling SimpleTimeDepthTransform::getZInterval_( bool isfrom,
						   bool makenice ) const
{
    const ZDomain::Info& from = SI().zDomainInfo();
    const ZDomain::Info& to = isfrom ? fromZDomainInfo() : toZDomainInfo();
    return getZInterval_( SI().zRange(true), from, to, makenice );
}


ZSampling SimpleTimeDepthTransform::getWorkZSampling( const ZSampling& zsamp,
						const ZDomain::Info& from,
						const ZDomain::Info& to ) const
{
    if ( !isOK() )
	return ZSampling::udf();

    const int nrsamples = zsamp.nrSteps();
    ZSampling ret = zsamp;
    if ( from.isTime() && to.isDepth() )
    {
	ret.start = tdmodel_->getDepth( ret.start );
	ret.stop = tdmodel_->getDepth( ret.stop );
    }
    else if ( from.isDepth() && to.isTime() )
    {
	ret.start = tdmodel_->getTime( ret.start );
	ret.stop = tdmodel_->getTime( ret.stop );
    }

    if ( to != from )
	ret.step = (ret.width()) / (nrsamples==0 ? 1 : nrsamples);

    return ret;
}


// Simplet2DTransform

SimpleT2DTransform::SimpleT2DTransform()
    : SimpleTimeDepthTransform(ZDomain::Time(),ZDomain::Depth())
{
}


SimpleT2DTransform::SimpleT2DTransform( const MultiID& id )
    : SimpleTimeDepthTransform(ZDomain::Time(),ZDomain::Depth(),id)
{
}


void SimpleT2DTransform::transformTrc( const TrcKey&,
				     const SamplingData<float>& sd,
				     int sz, float* res ) const
{
    doTransform( sd, fromZDomainInfo(), sz, res );
}



void SimpleT2DTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int sz, float* res ) const
{
    doTransform( sd, toZDomainInfo(), sz, res );
}


float SimpleT2DTransform::getGoodZStep() const
{
    return getZInterval_( false, true ).step;
}


Interval<float> SimpleT2DTransform::getZInterval( bool from ) const
{
    return getZInterval_( from, true );
}


Interval<float> SimpleT2DTransform::getZRange( bool time ) const
{
    Interval<float> zrg = SI().zRange( true );
    const bool survistime = SI().zIsTime();
    if ( time && survistime ) return zrg;

    const BinIDValue startbidval( 0, 0, zrg.start );
    const BinIDValue stopbidval( 0, 0, zrg.stop );
    if ( survistime && !time )
    {
	zrg.start = ZAxisTransform::transform( startbidval );
	zrg.stop = ZAxisTransform::transform( stopbidval );
    }
    else if ( !survistime && time )
    {
	zrg.start = ZAxisTransform::transformBack( startbidval );
	zrg.stop = ZAxisTransform::transformBack( stopbidval );
    }

    return zrg;
}



SimpleD2TTransform::SimpleD2TTransform()
    : SimpleTimeDepthTransform(ZDomain::Depth(),ZDomain::Time())
{
}


SimpleD2TTransform::SimpleD2TTransform( const MultiID& id )
    : SimpleTimeDepthTransform(ZDomain::Depth(),ZDomain::Time(),id)
{
}


void SimpleD2TTransform::transformTrc( const TrcKey&,
				     const SamplingData<float>& sd,
				     int sz, float* res ) const
{
    doTransform( sd, fromZDomainInfo(), sz, res );
}



void SimpleD2TTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int sz, float* res ) const
{
    doTransform( sd, toZDomainInfo(), sz, res );
}


float SimpleD2TTransform::getGoodZStep() const
{
    return getZInterval_( false, true ).step;
}


Interval<float> SimpleD2TTransform::getZInterval( bool from ) const
{
    return getZInterval_( from, true );
}


Interval<float> SimpleD2TTransform::getZRange( bool depth ) const
{
    Interval<float> zrg = SI().zRange( true );
    const bool survistime = SI().zIsTime();
    if ( depth && !survistime ) return zrg;

    const BinIDValue startbidval( 0, 0, zrg.start );
    const BinIDValue stopbidval( 0, 0, zrg.stop );
    if ( survistime && depth )
    {
	zrg.start = ZAxisTransform::transformBack( startbidval );
	zrg.stop = ZAxisTransform::transformBack( stopbidval );
    }
    else if ( !survistime && !depth )
    {
	zrg.start = ZAxisTransform::transform( startbidval );
	zrg.stop = ZAxisTransform::transform( stopbidval );
    }

    return zrg;
}


// SimpleTimeDepthAscIO

Table::FormatDesc* SimpleTimeDepthAscIO::getDesc( bool withunitfld )
{
    auto* fd = new Table::FormatDesc( "TimeDepthModel" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
						StringInpSpec(sKey::FloatUdf()),
						Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void SimpleTimeDepthAscIO::createDescBody( Table::FormatDesc* fd,
					   bool withunits )
{
    fd->bodyinfos_ += Table::TargetInfo::mkTimePosition( true, withunits );
    fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( true, withunits );
}


void SimpleTimeDepthAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
}


bool SimpleTimeDepthAscIO::get( od_istream& strm,
				SimpleTimeDepthModel& mdl ) const
{
    TypeSet<float> tvals, dvals;
    const UnitOfMeasure* twtascuom = Table::AscIO::getTimeUnit();
    const UnitOfMeasure* zascuom = Table::AscIO::getDepthUnit();
    const UnitOfMeasure* twtuom = SimpleTimeDepthModel::getTimeUnit();
    const UnitOfMeasure* zuom = SimpleTimeDepthModel::getDepthUnit();
    while( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const float tval = getFValue( 0 );
	const float dval = getFValue( 1 );
	if ( mIsUdf(tval) || mIsUdf(dval) )
	    continue;

	tvals += getConvertedValue( tval, twtascuom, twtuom );
	dvals += getConvertedValue( dval, zascuom, zuom );
    }

    mdl.setRawData( tvals, dvals );
    return true;
}
