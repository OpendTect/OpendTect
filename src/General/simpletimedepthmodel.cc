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

mDefSimpleTranslators(SimpleTimeDepthModel,"Simple Time-Depth Model",od,Mdl)

SimpleTimeDepthModel::SimpleTimeDepthModel()
    : TimeDepthModel()
{}


SimpleTimeDepthModel::SimpleTimeDepthModel( const MultiID& id )
    : TimeDepthModel()
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( ioobj )
	readFromFile( ioobj->fullUserExpr(true) );
}


bool SimpleTimeDepthModel::isOK() const
{ return rawtimes_.size() > 1 && rawtimes_.size() == rawdepths_.size(); }

void SimpleTimeDepthModel::setRawData( const TypeSet<float>& times,
					 const TypeSet<float>& depths )
{
    rawtimes_ = times;
    rawdepths_ = depths;
    setModel( rawdepths_.arr(), rawtimes_.arr(), rawtimes_.size() );
}


bool SimpleTimeDepthModel::readFromFile( const char* fnm )
{
    rawtimes_.erase();
    rawdepths_.erase();

    od_istream strm( fnm );
    ascistream astrm( strm );
    astrm.next();
    IOPar pars( astrm );
    int sz = 0;
    if ( !pars.get(sKey::NrValues(),sz) || sz < 2 )
	return false;

    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZStorageUnit();
    BufferString zunit;
    const UnitOfMeasure* outzuom = nullptr;
    if ( pars.get(sKey::DepthUnit(),zunit) && !zunit.isEmpty() )
	outzuom =  UoMR().get( zunit );

    const UnitOfMeasure* tmuom = UnitOfMeasure::surveyDefTimeStorageUnit();
    BufferString tmunit;
    const UnitOfMeasure* outtmuom = nullptr;
    if ( pars.get(sKey::DepthUnit(),tmunit) && !tmunit.isEmpty() )
	outtmuom =  UoMR().get( tmunit );

    float tval, dval;
    for ( int idx=0; idx<sz; idx++ )
    {
	strm >> tval >> dval;
	if( !strm.isOK() )
	    break;

	rawtimes_ += getConvertedValue( tval, tmuom, outtmuom );
	rawdepths_ += getConvertedValue( dval, zuom, outzuom );
    }

    setModel( rawdepths_.arr(), rawtimes_.arr(), rawtimes_.size() );
    return isOK();
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
    pars.set( sKey::TimeUnit(),
	    UoMR().getInternalFor(
			UnitOfMeasureRepository::PropType::Time)->getLabel() );
    pars.set( sKey::DepthUnit(),
			    UnitOfMeasure::surveyDefDepthUnit()->getLabel() );
    pars.set( sKey::NrValues(), rawtimes_.size() );
    pars.write( strm, "Simple Time-Depth Model" );
    for ( int idx=0; idx<rawtimes_.size(); idx++ )
	strm << rawtimes_[idx] << "\t" << rawdepths_[idx] << od_endl;

    return true;
}


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
    return tdmodel_ && tdmodel_->isOK();
}


void SimpleTimeDepthTransform::doTransform( const SamplingData<float>& sd,
				    int ressz, float* res, bool t2d ) const
{
    const bool survistime = SI().zIsTime();
    const bool depthsinfeet = SI().depthsInFeet();

    for ( int idx=0; idx<ressz; idx++ )
    {
	if ( sd.isUdf() )
	{
	    res[idx] = mUdf(float);
	    continue;
	}

	float inp = sd.atIndex( idx );
	if ( t2d )
	{
	    res[idx] = tdmodel_->getDepth( inp );
	    if ( survistime && depthsinfeet ) res[idx] *= mToFeetFactorF;
	}
	else
	{
	    if ( survistime && depthsinfeet ) inp *= mFromFeetFactorF;
	    res[idx] = tdmodel_->getTime( inp );
	}
    }
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

    if ( !setID(MultiID(tozdomaininfo_.getID() ) ) )
	return false;

    return true;
}



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
				     int ressz, float* res ) const
{
    doTransform( sd, ressz, res, true );
}



void SimpleT2DTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int ressz, float* res ) const
{
    doTransform( sd, ressz, res, false );
}


float SimpleT2DTransform::getGoodZStep() const
{
    if ( !SI().zIsTime() )
	return SI().zRange(true).step;

    Interval<float> zrg = SI().zRange( true );
    zrg.start = transform( BinIDValue(0,0,zrg.start) );
    zrg.stop = transform( BinIDValue(0,0,zrg.stop) );
    const int userfac = toZDomainInfo().userFactor();
    const int nrsteps = SI().zRange( true ).nrSteps();
    float zstep = zrg.width() / (nrsteps==0 ? 1 : nrsteps);
    zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
    zstep /= userfac;
    return zstep;
}


Interval<float> SimpleT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg = getZRange( time );
    const float step = getGoodZStep();
    const int userfac = toZDomainInfo().userFactor();
    const int stopidx = zrg.indexOnOrAfter( zrg.stop, step );
    zrg.stop = zrg.atIndex( stopidx, step );
    zrg.stop = mCast(float,mNINT32(zrg.stop*userfac))/userfac;
    return zrg;
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
				     int ressz, float* res ) const
{
    doTransform( sd, ressz, res, false );
}



void SimpleD2TTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int ressz, float* res ) const
{
    doTransform( sd, ressz, res, true );
}


float SimpleD2TTransform::getGoodZStep() const
{
    if ( SI().zIsTime() )
	return SI().zRange(true).step;

    Interval<float> zrg = SI().zRange( true );
    zrg.start = transform( BinIDValue(0,0,zrg.start) );
    zrg.stop = transform( BinIDValue(0,0,zrg.stop) );
    const int userfac = toZDomainInfo().userFactor();
    const int nrsteps = SI().zRange( true ).nrSteps();
    float zstep = zrg.width() / (nrsteps==0 ? 1 : nrsteps);
    zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
    zstep /= userfac;
    return zstep;
}


Interval<float> SimpleD2TTransform::getZInterval( bool depth ) const
{
    Interval<float> zrg = getZRange( depth );
    const float step = getGoodZStep();
    const int userfac = toZDomainInfo().userFactor();
    const int stopidx = zrg.indexOnOrAfter( zrg.stop, step );
    zrg.stop = zrg.atIndex( stopidx, step );
    zrg.stop = mCast(float,mNINT32(zrg.stop*userfac))/userfac;
    return zrg;
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



Table::FormatDesc* SimpleTimeDepthAscIO::getDesc( bool withunitfld )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "TimeDepthModel" );
    fd->headerinfos_ +=
	new Table::TargetInfo( "Undefined Value",
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
    while( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float tval = getFValue( 0 );
	// time values are in internal standard i.e. (s) at this point
	const UnitOfMeasure* inpuom = UnitOfMeasure::surveyDefTimeStorageUnit();
	const UnitOfMeasure* outuom = UnitOfMeasure::surveyDefTimeUnit();
	convValue( tval, inpuom, outuom );
	const float dval = getFValue( 1 );
	if ( mIsUdf(tval) || mIsUdf(dval) )
	    continue;

	tvals += tval;
	dvals += dval;
    }

    mdl.setRawData( tvals, dvals );
    return true;
}
