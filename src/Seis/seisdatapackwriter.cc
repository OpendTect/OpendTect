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
#include "seisdatapack.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "uistrings.h"


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
    obtainDP();
    getPosInfo();

    if ( compidxs_.isEmpty() )
    {
	for ( int idx=0; idx<dp_->nrComponents(); idx++ )
	    compidxs_ += idx;
    }

    const int startz =
	mNINT32(dp_->sampling().zsamp_.start/dp_->sampling().zsamp_.step);
    zrg_ = Interval<int>( startz, startz+dp_->sampling().nrZ()-1 );

    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    writer_ = ioobj ? new SeisTrcWriter( ioobj ) : 0;
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    releaseDP();
    delete trc_;
    delete writer_;
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


void SeisDataPackWriter::setSelection( const TrcKeySampling& hrg,
				    const Interval<int>& zrg )
{
    zrg_ = zrg; tks_ = hrg;

    iterator_.setSampling( hrg );
    totalnr_ = posinfo_ ? posinfo_->totalSizeInside( hrg )
			: mCast(int,hrg.totalNr());
}


od_int64 SeisDataPackWriter::totalNr() const
{
    return totalnr_;
}


int SeisDataPackWriter::nextStep()
{
    const StepInterval<float> survrg = SI().zRange( true ); 
    const StepInterval<int> cubezrgidx // real -> index
			    ( mNINT32(dp_->sampling().zsamp_.start/survrg.step),
			      mNINT32(dp_->sampling().zsamp_.stop/survrg.step),
			      mNINT32(dp_->sampling().zsamp_.step/survrg.step));
			      

    if ( !trc_ )
    {
	if ( !writer_ || dp_->isEmpty() )
	    return ErrorOccurred();

	const int trcsz = cubezrgidx.nrSteps() + 1;
	trc_ = new SeisTrc( trcsz );

	trc_->info().sampling.start = dp_->sampling().zsamp_.start;
	trc_->info().sampling.step = dp_->sampling().zsamp_.step;
	trc_->info().nr = 0;

	BufferStringSet compnames;
	compnames.add( dp_->getComponentName() );
	for ( int idx=1; idx<compidxs_.size(); idx++ )
	{
	    trc_->data().addComponent( trcsz, DataCharacteristics() );
	    compnames.add( dp_->getComponentName(idx) );
	}

	SeisTrcTranslator* transl = writer_->seisTranslator();
	if ( transl ) transl->setComponentNames( compnames );
    }

    BinID currentpos;
    if ( !iterator_.next( currentpos ) )
	return Finished();

    const TrcKeySampling& hs = dp_->sampling().hsamp_;

    trc_->info().binid = currentpos;
    trc_->info().coord = SI().transform( currentpos );
    const int inl = currentpos.inl();
    const int crl = currentpos.crl();
    if ( posinfo_ && !posinfo_->includes(inl,crl) )
	return MoreToDo();

    const int inlidx = hs.inlRange().nearestIndex( inl );
    const int crlidx = hs.crlRange().nearestIndex( crl );

    for ( int idx=0; idx<compidxs_.size(); idx++ )
    {
	for ( int zidx=0; zidx<=cubezrgidx.nrSteps(); zidx++ )
	{
	    const int zsample = zidx+zrg_.start;
	    const int cubesample = zsample - cubezrgidx.start;

	    const float value = cubezrgidx.includes( zsample, false )
		? dp_->data(compidxs_[idx]).get(inlidx,crlidx,cubesample)
		: mUdf(float);

	    trc_->set( zidx, value, idx );
	}
    }

    if ( !writer_->put(*trc_) )
	return ErrorOccurred();

    nrdone_++;
    trc_->info().nr++;
    return MoreToDo();
}
