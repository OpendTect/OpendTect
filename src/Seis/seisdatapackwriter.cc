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
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "uistrings.h"

#include "hiddenparam.h"

HiddenParam<SeisDataPackWriter,StepInterval<int>* > cubezrgidx_(0);
HiddenParam<SeisDataPackWriter,ObjectSet<Scaler>* > seisdpwrrcompscalers_(0);


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
    cubezrgidx_.setParam( this, new StepInterval<int>( 0,0,0 ) );
    ObjectSet<Scaler>* compscalers = new ObjectSet<Scaler>;
    compscalers->allowNull( true );
    seisdpwrrcompscalers_.setParam( this, compscalers );
    obtainDP();
    getPosInfo();

    if ( compidxs_.isEmpty() )
    {
	for ( int idx=0; idx<dp_->nrComponents(); idx++ )
	{
	    compidxs_ += idx;
	    *compscalers += 0;
	}
    }
    else
    {
	for ( int idx=0; idx<compidxs_.size(); idx++ )
	    *compscalers += 0;
    }

    const int startz =
	mNINT32(dp_->sampling().zsamp_.start/dp_->sampling().zsamp_.step);
    zrg_ = Interval<int>( startz, startz+dp_->sampling().nrZ()-1 );
    setCubeIdxRange();

    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    writer_ = ioobj ? new SeisTrcWriter( ioobj ) : 0;
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    releaseDP();
    delete trc_;
    delete writer_;
    delete cubezrgidx_.getParam( this );
    cubezrgidx_.removeParam( this );
    ObjectSet<Scaler>* compscalers = seisdpwrrcompscalers_.getParam( this );
    deepErase( *compscalers );
    delete compscalers;
    seisdpwrrcompscalers_.removeParam( this );
}


void SeisDataPackWriter::getPosInfo()
{
    const PosInfo::CubeData* pi = dp_->getTrcsSampling();
    if ( pi && !pi->isFullyRectAndReg() )
    {
	posinfo_ = pi;
	totalnr_ = posinfo_->totalSizeInside( tks_ );
    }
}


void SeisDataPackWriter::setComponentScaler( const Scaler& scaler, int compidx )
{
    if ( scaler.isEmpty() )
	return;

    ObjectSet<Scaler>* compscalers = seisdpwrrcompscalers_.getParam( this );
    for ( int idx=0; idx<=compidx; idx++ )
    {
	if ( !compscalers->validIdx(idx) )
	    *compscalers += 0;
    }

    delete compscalers->replace( compidx, scaler.clone() );
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

    getPosInfo();
    nrdone_ = 0;
    setSelection( dp_->sampling().hsamp_, zrg_ );
}


void SeisDataPackWriter::obtainDP()
{
    DPM( DataPackMgr::SeisID() ).obtain( dp_->id() );
}


void SeisDataPackWriter::releaseDP()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
}


void SeisDataPackWriter::setCubeIdxRange()
{
    const float zstep = SI().zRange( false ).step;
    StepInterval<int>* cubezrgidx = cubezrgidx_.getParam( this );
    cubezrgidx->set( mNINT32(dp_->sampling().zsamp_.start/zstep),
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
}


od_int64 SeisDataPackWriter::totalNr() const
{
    return totalnr_;
}


bool SeisDataPackWriter::setTrc()
{
    if ( !writer_ || dp_->isEmpty() )
	return false;

    const int trcsz = cubezrgidx_.getParam(this)->nrSteps() + 1;
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

    SeisTrcTranslator* transl = writer_->seisTranslator();
    if ( transl ) transl->setComponentNames( compnames );

    return true;
}


int SeisDataPackWriter::nextStep()
{
    if ( !trc_ && !setTrc() )
	return ErrorOccurred();

    BinID currentpos;
    if ( !iterator_.next(currentpos) )
	return Finished();

    ObjectSet<Scaler>& compscalers = *seisdpwrrcompscalers_.getParam( this );
    const TrcKeySampling& hs = dp_->sampling().hsamp_;
    const od_int64 posidx = iterator_.curIdx();
    if ( posinfo_ && !posinfo_->isValid(posidx,hs) )
	return MoreToDo();

    trc_->info().binid = currentpos;
    trc_->info().coord = hs.toCoord( currentpos );
    const int inlpos = hs.lineIdx( currentpos.inl() );
    const int crlpos = hs.trcIdx( currentpos.crl() );
    const int trcsz = trc_->size();
    const StepInterval<int>* cubezrgidx = cubezrgidx_.getParam( this );
    int zsample = zrg_.start;
    int cubesample = zsample - cubezrgidx->start;
    const od_int64 offset = dp_->data().info().
					getOffset( inlpos, crlpos, cubesample );
    float value = mUdf(float);
    for ( int idx=0; idx<compidxs_.size(); idx++ )
    {
	const Array3D<float>& outarr = dp_->data( compidxs_[idx] );
	const float* dataptr = outarr.getData();
	const Scaler* scaler = compscalers[idx];
	dataptr += offset;
	zsample = zrg_.start;
	cubesample = zsample - cubezrgidx->start;
	for ( int zidx=0; zidx<trcsz; zidx++, zsample++ )
	{
	    if ( cubezrgidx->includes(zsample,false) )
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
    trc_->info().nr++;
    return MoreToDo();
}
