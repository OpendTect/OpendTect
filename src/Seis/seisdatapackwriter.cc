/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seisdatapackwriter.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"


SeisDataPackWriter::SeisDataPackWriter( const MultiID& mid,
				  const RegularSeisDataPack& dp,
				  const TypeSet<int>& compidxs )
    : Executor( "Attribute volume writer" )
    , nrdone_( 0 )
    , tks_( dp.sampling().hsamp_ )
    , totalnr_( (int) dp.sampling().hsamp_.totalNr() )
    , dp_( &dp )
    , iterator_( dp.sampling().hsamp_ )
    , mid_( mid )
    , posinfo_(0)
    , compidxs_( compidxs )
    , trc_( 0 )
{
    compscalers_.allowNull( true );
    obtainDP();

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

    const int startz =
	mNINT32(dp_->sampling().zsamp_.start/dp_->sampling().zsamp_.step);
    zrg_ = Interval<int>( startz, startz+dp_->sampling().nrZ()-1 );
    setCubeIdxRange();

    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    writer_ = ioobj ? new SeisTrcWriter( ioobj ) : 0;
    is2d_ = writer_->is2D();
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    releaseDP();
    delete trc_;
    delete writer_;
    deepErase( compscalers_ );
}


void SeisDataPackWriter::getPosInfo()
{
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

    const Seis::ObjectSummary summary( mid_ );
    if ( !summary.is2D() )
	return;

    const IOObj* outioobj = summary.getFullInformation().ioObj();
    BufferString type;
    if ( !outioobj || !outioobj->pars().get(sKey::Type(),type) ||
	 type != BufferString(sKey::Steering()) )
	return;

    const Survey::Geometry* geom = Survey::GM().getGeometry(
					dp_->sampling().hsamp_.getGeomID() );
    if ( !geom || !geom->as2D() )
	return;

    double trcdist = geom->as2D()->averageTrcDist();
    const UnitOfMeasure* feetuom = UoMR().get( "Feet" );
    if ( feetuom && SI().xyInFeet() )
	trcdist = feetuom->getSIValue( trcdist );

    double zstep = dp_->sampling().zsamp_.step;
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const ZDomain::Def& zdef = SI().zDomain();
    if ( zuom && zdef.isDepth() )
	zstep = zuom->getSIValue( zstep );

    const UnitOfMeasure* zdipuom = zdef.isDepth() ? UoMR().get( "Millimeters" )
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
	releaseDP();
	dp_ = &dp;
	obtainDP();
    }

    nrdone_ = 0;
    setSelection( dp_->sampling().hsamp_, zrg_ );
}


void SeisDataPackWriter::obtainDP()
{
    if ( !dp_ || !DPM( DataPackMgr::SeisID() ).obtain(dp_->id()) )
    {
	releaseDP();
	return;
    }

    getPosInfo();
}


void SeisDataPackWriter::releaseDP()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = 0;
    posinfo_ = 0;
}


void SeisDataPackWriter::setCubeIdxRange()
{
    const float zstep = SI().zRange( false ).step;
    cubezrgidx_.set( mNINT32(dp_->sampling().zsamp_.start/zstep),
			 mNINT32(dp_->sampling().zsamp_.stop/zstep),
			 mNINT32(dp_->sampling().zsamp_.step/zstep) );
}


void SeisDataPackWriter::setSelection( const TrcKeySampling& hrg,
				    const Interval<int>& zrg )
{
    zrg_ = zrg; tks_ = hrg;
    setCubeIdxRange();

    iterator_.setSampling( hrg );
    totalnr_ = posinfo_ ? posinfo_->totalSizeInside( hrg )
			: mCast(int,hrg.totalNr());
    Seis::SelData* seldata = new Seis::RangeSelData( tks_ );

    // Workaround for v6.2. Not needed in later versions.
    if ( is2d_ )
	seldata->setGeomID( tks_.start_.lineNr() );

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

    const int trcsz = cubezrgidx_.nrSteps() + 1;
    trc_ = new SeisTrc( trcsz );

    trc_->info().sampling.start = dp_->sampling().zsamp_.start;
    trc_->info().sampling.step = dp_->sampling().zsamp_.step;

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
    BinID currentpos;
    if ( !iterator_.next(currentpos) )
	return Finished();

    const TrcKeySampling& hs = dp_->sampling().hsamp_;
    if ( posinfo_ && !posinfo_->isValid(posidx,hs) )
	return MoreToDo();

    trc_->info().setBinID( currentpos );
    trc_->info().nr = is2d_ ? currentpos.trcNr() : 0;
    trc_->info().binid = currentpos;
    trc_->info().coord = hs.toCoord( currentpos );
    const int inlpos = hs.lineIdx( currentpos.inl() );
    const int crlpos = hs.trcIdx( currentpos.crl() );
    const int trcsz = trc_->size();
    int zsample = zrg_.start;
    int cubesample = zsample - cubezrgidx_.start;
    const od_int64 offset = dp_->data().info().
					getOffset( inlpos, crlpos, cubesample );
    float value = mUdf(float);
    for ( int idx=0; idx<compidxs_.size(); idx++ )
    {
	const Array3D<float>& outarr = dp_->data( compidxs_[idx] );
	const float* dataptr = outarr.getData();
	const Scaler* scaler = compscalers_[idx];
	if ( dataptr ) dataptr += offset;
	zsample = zrg_.start;
	cubesample = zsample - cubezrgidx_.start;
	for ( int zidx=0; zidx<trcsz; zidx++, zsample++ )
	{
	    if ( cubezrgidx_.includes(zsample,false) )
	    {
		if ( dataptr )
		    value = *dataptr++;
		else
		{
		    value = outarr.get( inlpos, crlpos, cubesample );
		    cubesample++;
		}

		if ( scaler )
		    value = mCast(float,scaler->scale( value ));
	    }
	    else
		value = mUdf(float);

	    trc_->set( zidx, value, idx );
	}
    }

    if ( !writer_->put(*trc_) )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}
