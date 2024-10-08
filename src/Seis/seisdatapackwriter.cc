/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapackwriter.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"


SeisDataPackWriter::SeisDataPackWriter( const MultiID& mid,
				  const RegularSeisDataPack& dp,
				  const TypeSet<int>& compidxs )
    : Executor( "Attribute volume writer" )
    , compidxs_( compidxs )
    , mid_( mid )
    , dp_( &dp )
    , nrdone_( 0 )
    , totalnr_( (int) dp.sampling().hsamp_.totalNr() )
    , iterator_( dp.sampling().hsamp_ )
    , posinfo_(nullptr)
    , trc_(nullptr)
    , tks_( dp.sampling().hsamp_ )
{
    getPosInfo();
    compscalers_.allowNull( true );

    if ( compidxs_.isEmpty() )
    {
	for ( int idx=0; idx<dp_->nrComponents(); idx++ )
	{
	    compidxs_ += idx;
	    compscalers_ += 0;
	}
    }
    else
    {
	for ( int idx=0; idx<compidxs_.size(); idx++ )
	    compscalers_ += 0;
    }

    zrg_ = Interval<int>( 0, dp_->sampling().nrZ()-1 );
    cubezrgidx_.set( zrg_, 1 );

    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    writer_ = ioobj ? new SeisTrcWriter( *ioobj ) : nullptr;
    is2d_ = writer_->is2D();
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    delete trc_;
    delete writer_;
    deepErase( compscalers_ );
}


void SeisDataPackWriter::getPosInfo()
{
    if ( !dp_ )
	return;

    const PosInfo::CubeData* pi = dp_->getTrcsSampling();
    posinfo_ = pi;
    if ( pi && !pi->isFullyRectAndReg() )
	totalnr_ = posinfo_->totalSizeInside( tks_ );
}


void SeisDataPackWriter::setComponentScaler( const Scaler& scaler, int compidx )
{
    if ( scaler.isEmpty() )
	return;

    for ( int idx=0; idx<=compidx; idx++ )
    {
	if ( !compscalers_.validIdx(idx) )
	    compscalers_ += 0;
    }

    delete compscalers_.replace( compidx, scaler.clone() );
    adjustSteeringScaler( compidx );
}


void SeisDataPackWriter::adjustSteeringScaler( int compidx )
{
    if ( !writer_ || !dp_ || !dp_->is2D() ||
	 !compscalers_[compidx] || compscalers_[compidx]->isEmpty() )
	return;

    const SeisIOObjInfo objinfo( mid_ );
    if ( !objinfo.isOK() || !objinfo.is2D() )
	return;

    const IOObj* outioobj = objinfo.ioObj();
    BufferString type;
    if ( !outioobj || !outioobj->pars().get(sKey::Type(),type) ||
	 type != BufferString(sKey::Steering()) )
	return;

    const Pos::GeomID geomid = dp_->sampling().hsamp_.getGeomID();
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    if ( !geom || !geom->as2D() )
	return;

    double trcdist = geom->as2D()->averageTrcDist();
    const UnitOfMeasure* feetuom = UoMR().get( "Feet" );
    if ( feetuom && SI().xyInFeet() )
	trcdist = feetuom->getSIValue( trcdist );

    double zstep = dp_->sampling().zsamp_.step_;
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const ZDomain::Def& zdef = SI().zDomain();
    if ( zuom && zdef.isDepth() )
	zstep = zuom->getSIValue( zstep );

    const UnitOfMeasure* zdipuom = zdef.isDepth() ? UoMR().get( "Millimeter" )
						  : UoMR().get( "Microseconds");
    const UnitOfMeasure* targetzuom = zdipuom;
    if ( targetzuom )
	zstep = targetzuom->getUserValueFromSI( zstep );

    const double scalefactor = zstep / trcdist;
    const LinScaler dipscaler( 0., scalefactor );
    delete compscalers_.replace( compidx, dipscaler.clone() );
}


od_int64 SeisDataPackWriter::nrDone() const
{
    return nrdone_;
}


uiString SeisDataPackWriter::uiMessage() const
{
    if ( !writer_ )
	return uiStrings::phrCannotWrite( tr("the output data to disk.") );

    return tr("Writing seismic volume \'%1\'").arg( writer_->ioObj()->uiName());
}


void SeisDataPackWriter::setNextDataPack( const RegularSeisDataPack& dp )
{
    if ( dp_ != &dp )
    {
	dp_ = &dp;
	getPosInfo();
    }

    nrdone_ = 0;
    zrg_ = Interval<int>( 0, dp_->sampling().nrZ()-1 );
    setSelection( dp_->sampling().hsamp_, cubezrgidx_ );
}


void SeisDataPackWriter::setCubeIdxRange()
{
}


void SeisDataPackWriter::setSelection( const TrcKeySampling& hrg,
				       const Interval<int>& cubezrgidx )
{
    tks_ = hrg;
    if ( !zrg_.includes(cubezrgidx) )
    {
	pErrMsg("Invalid selection");
	cubezrgidx_.setUdf();
	return;
    }

    cubezrgidx_.set( cubezrgidx, 1 );

    iterator_.setSampling( hrg );
    totalnr_ = posinfo_ ? posinfo_->totalSizeInside( hrg )
			: mCast(int,hrg.totalNr());
    Seis::SelData* seldata = new Seis::RangeSelData( tks_ );

    // Workaround for v6.2. Not needed in later versions.
    if ( is2d_ )
	seldata->setGeomID( Pos::GeomID(tks_.start_.lineNr()) );

    if ( writer_ )
	writer_->setSelData( seldata );
}


od_int64 SeisDataPackWriter::totalNr() const
{
    return totalnr_;
}


bool SeisDataPackWriter::setTrc()
{
    if ( !writer_ || dp_->isEmpty() )
	return false;

    if ( cubezrgidx_ == Interval<int>::udf() )
    {
	pErrMsg("Invalid selection");
	return false;
    }

    const int trcsz = cubezrgidx_.nrSteps() + 1;
    trc_ = new SeisTrc( trcsz );

    trc_->info().sampling_.start_ =
		dp_->sampling().zsamp_.atIndex( cubezrgidx_.start_ );
    trc_->info().sampling_.step_ =
		dp_->sampling().zsamp_.step_ * cubezrgidx_.step_;

    BufferStringSet compnames;
    compnames.add( dp_->getComponentName() );
    for ( int idx=1; idx<compidxs_.size(); idx++ )
    {
	trc_->data().addComponent( trcsz, DataCharacteristics() );
	compnames.add( dp_->getComponentName(idx) );
    }

    writer_->setComponentNames( compnames );
    return true;
}


int SeisDataPackWriter::nextStep()
{
    if ( !trc_ && !setTrc() )
	return ErrorOccurred();

    const od_int64 posidx = iterator_.curIdx();
    TrcKey currentpos;
    if ( !iterator_.next(currentpos) )
	return Finished();

    const TrcKeySampling& hs = dp_->sampling().hsamp_;
    if ( posinfo_ && !posinfo_->isValid(posidx,hs) )
	return MoreToDo();

    trc_->info().setTrcKey( currentpos ).calcCoord();
    const int inlpos = hs.lineIdx( currentpos.inl() );
    const int crlpos = hs.trcIdx( currentpos.crl() );
    const int trcsz = trc_->size();
    int cubesample = cubezrgidx_.start_;
    const od_int64 offset = dp_->data().info().
					getOffset( inlpos, crlpos, cubesample );
    float value = mUdf(float);
    for ( int compidx=0; compidx<compidxs_.size(); compidx++ )
    {
	const Array3D<float>& outarr = dp_->data( compidxs_[compidx] );
	const float* dataptr = outarr.getData();
	const Scaler* scaler = compscalers_[compidx];
	if ( dataptr ) dataptr += offset;
        cubesample = cubezrgidx_.start_;
	for ( int idz=0; idz<trcsz; idz++ )
	{
	    value = dataptr ? *dataptr++
			    : outarr.get( inlpos, crlpos, cubesample++ );

	    if ( scaler )
		value = mCast(float,scaler->scale( value ));

	    trc_->set( idz, value, compidx );
	}
    }

    if ( !writer_->put(*trc_) )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}
