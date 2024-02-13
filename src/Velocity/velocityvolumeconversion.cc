/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityvolumeconversion.h"

#include "binidvalset.h"
#include "ioman.h"
#include "ioobj.h"
#include "ioobjtags.h"
#include "posinfo.h"
#include "seisbounds.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "seispacketinfo.h"
#include "seistype.h"
#include "sorting.h"
#include "uistrings.h"
#include "timedepthconv.h"
#include "varlenarray.h"
#include "veldescimpl.h"
#include "velocitycalc.h"

namespace Vel
{

const char* VolumeConverterNew::sKeyInput() { return sKey::Input(); }
const char* VolumeConverterNew::sKeyOutput() { return sKey::Output(); }

VolumeConverterNew::VolumeConverterNew( const IOObj& input, const IOObj& output,
					const TrcKeySampling& ranges,
					const VelocityDesc& desc, double srd,
					const UnitOfMeasure* srduom )
    : msg_(tr("Converting velocities"))
    , input_(input)
    , output_(output)
    , velinpdesc_(*new VelocityDesc)
    , veloutpdesc_(*new VelocityDesc(desc))
    , srd_(srd)
    , srduom_(srduom)
    , tks_(ranges)
{
    velinpdesc_.usePar( input_.pars() );
    msg_ = tr("Converting velocities from %1 to %2")
		.arg( VelocityDesc::toString(velinpdesc_.type_) )
		.arg( VelocityDesc::toString(veloutpdesc_.type_) );
    setRanges();
}


VolumeConverterNew::~VolumeConverterNew()
{
    delete &velinpdesc_;
    delete &veloutpdesc_;
    delete reader_;
    delete writer_;
    delete sequentialwriter_;
}


uiString VolumeConverterNew::uiMessage() const
{
    return msg_;
}


uiString VolumeConverterNew::uiNrDoneText() const
{
    return sTrcFinished();
}


od_int64 VolumeConverterNew::nrIterations() const
{
    return totalnr_;
}


void VolumeConverterNew::setRanges()
{
    const SeisIOObjInfo info( input_ );
    if ( !info.isOK() )
	return;

    const Seis::GeomType geomtype = info.geomType();
    if ( Seis::is2D(geomtype) )
    {
	StepInterval<int> trcrg;
	ZSampling zrg;
	if ( !info.getRanges(tks_.getGeomID(),trcrg,zrg) )
	    return;

	totalnr_ = trcrg.nrSteps() + 1;
    }
    else
    {
	TrcKeyZSampling storhrg;
	if ( !info.getRanges(storhrg) )
	    return;

	tks_.limitTo( storhrg.hsamp_ );
	SeisTrcReader rdr( input_, &geomtype );
	PosInfo::CubeData posinfo;
	if ( !rdr.prepareWork(Seis::Scan) || !rdr.get3DGeometryInfo(posinfo) )
	    return;

	totalnr_ = posinfo.totalSizeInside( tks_ );
    }
}


bool VolumeConverterNew::doPrepare( int nrthreads )
{
    if ( totalnr_ < 0 )
    {
	msg_ = tr("No positions to process");
	return false;
    }

    if ( !velinpdesc_.isVelocity() || !veloutpdesc_.isVelocity() ||
	 velinpdesc_.type_ == veloutpdesc_.type_ )
    {
	msg_ = tr("Input/output velocities are not interval, RMS, or Avg "
		     "or are identical.");
	return false;
    }

    delete reader_;
    reader_ = new SeisTrcReader( input_ );
    if ( !reader_->prepareWork() )
    {
	deleteAndNullPtr( reader_ );
	msg_ = reader_->errMsg();
	return false;
    }

    reader_->setSelData( new Seis::RangeSelData(tks_) );

    zdomaininfo_ = &reader_->zDomain();
    delete writer_;
    writer_ = new SeisTrcWriter( output_ );
    delete sequentialwriter_;
    sequentialwriter_ = new SeisSequentialWriter( writer_ );

    return true;
}


bool VolumeConverterNew::doWork( od_int64, od_int64, int threadidx )
{
    char res = 1;
    SeisTrc trc;

    lock_.lock();
    res = getNewTrace( trc, threadidx );
    lock_.unLock();

    while ( res==1 )
    {
	if ( !shouldContinue() )
	    return false;

	auto* outputtrc = new SeisTrc( trc );
	outputtrc->updateVelocities( velinpdesc_, veloutpdesc_,
				     *zdomaininfo_, srd_, srduom_ );
	sequentialwriter_->submitTrace( outputtrc, true );
	addToNrDone( 1 );

	Threads::MutexLocker lock( lock_ );
	res = getNewTrace( trc, threadidx );
    }

    return res==0;
}


bool VolumeConverterNew::doFinish( bool success )
{
    zdomaininfo_= nullptr;
    deleteAndNullPtr( reader_ );
    if ( !sequentialwriter_->finishWrite() )
	success = false;

    deleteAndNullPtr( sequentialwriter_ );
    deleteAndNullPtr( writer_ );

    if ( !success )
	return false;

    const IOPar& inpiop = input_.pars();
    IOPar& outiop = output_.pars();
    veloutpdesc_.fillPar( outiop );
    ::Interval<float> velrg;
    VelocityStretcherNew::getRange( inpiop, velinpdesc_, true, velrg );
    VelocityStretcherNew::setRange( velrg, veloutpdesc_, true, outiop );
    VelocityStretcherNew::getRange( inpiop, velinpdesc_, false, velrg );
    VelocityStretcherNew::setRange( velrg, veloutpdesc_, false, outiop );
    if ( !IOM().commitChanges(output_) )
    {
	msg_ = uiStrings::phrCannotWrite( tr("velocity information") );
	return false;
    }

    return success;
}


char VolumeConverterNew::getNewTrace( SeisTrc& trc, int threadidx )
{
    if ( !reader_ )
	return 0;

    int res = 2;
    while ( res==2 || !tks_.includes(trc.info().binID()) )
	res = reader_->get( trc.info() );

    if ( res==1 )
    {
	if ( !reader_->get(trc) )
	    return -1;

	sequentialwriter_->announceTrace( trc.info().binID() );
	return 1;
    }

    if ( res==-1 )
	msg_ = uiStrings::sCantReadInp();

    deleteAndNullPtr( reader_ );

    return mCast( char, res );
}


// Old task implementation
mStartAllowDeprecatedSection

const char* VolumeConverter::sKeyInput() { return sKey::Input(); }
const char* VolumeConverter::sKeyOutput() { return sKey::Output(); }

VolumeConverter::VolumeConverter( const IOObj& input, const IOObj& output,
				  const TrcKeySampling& ranges,
				  const VelocityDesc& desc )
    : tks_( ranges )
    , veloutpdesc_( desc )
    , input_( input.clone() )
    , output_( output.clone() )
    , reader_( 0 )
    , writer_( 0 )
    , sequentialwriter_(0)
{
    reader_ = new SeisTrcReader( input );
    if ( !reader_->prepareWork() )
    {
	deleteAndNullPtr( reader_ );
	errmsg_ = uiStrings::sCantReadInp();
	return;
    }

    const SeisPacketInfo& packetinfo =
	    reader_->seisTranslator()->packetInfo();

    if ( packetinfo.cubedata )
    {
	BinIDValueSet bivs( 0, false );
	bivs.add( *packetinfo.cubedata );
	bivs.remove( tks_, false );
	totalnr_ = bivs.totalSize();
    }
    else
    {
	totalnr_ = tks_.totalNr();
    }

    Seis::SelData* seldata = new Seis::RangeSelData( tks_ );
    reader_->setSelData( seldata );
}


VolumeConverter::~VolumeConverter()
{
    delete input_;
    delete output_;

    if ( writer_ ) delete writer_;
    if ( sequentialwriter_ ) delete sequentialwriter_;
    if ( reader_ ) delete reader_;
}

bool VolumeConverter::doFinish( bool res )
{
    delete reader_;
    reader_ = 0;

    if ( !sequentialwriter_->finishWrite() )
	res = false;

    delete sequentialwriter_;
    sequentialwriter_ = 0;

    delete writer_;
    writer_ = 0;

    return res;
}


bool VolumeConverter::doPrepare( int nrthreads )
{
    if ( !errmsg_.getFullString().isEmpty() )
	return false;

    if ( !input_ || !output_ )
    {
	errmsg_ = tr("Either input or output cannot be found");
	return false;
    }

    delete writer_;
    writer_ = 0;

    if ( !GetVelocityTag( *input_, velinpdesc_ ) )
    {
	errmsg_ = tr("Cannot read velocity information on input.");
	return false;
    }

    if ( !SetVelocityTag( *output_, veloutpdesc_ ) )
    {
	errmsg_ = tr("Cannot write velocity information on output");
	return false;
    }

    if ( ( velinpdesc_.type_ != VelocityDesc::Interval &&
	 velinpdesc_.type_ != VelocityDesc::RMS &&
	 velinpdesc_.type_ != VelocityDesc::Avg ) ||
         ( veloutpdesc_.type_ != VelocityDesc::Interval &&
	 veloutpdesc_.type_ != VelocityDesc::RMS &&
	 veloutpdesc_.type_ != VelocityDesc::Avg ) ||
	 velinpdesc_.type_ == veloutpdesc_.type_ )
    {
	errmsg_ = tr("Input/output velocities are not interval, RMS, or Avg "
	             "or are identical.");
	return false;
    }

    //Check input Vel

    if ( !reader_ )
    {
	reader_ = new SeisTrcReader( *input_ );
	if ( !reader_->prepareWork() )
	{
	    deleteAndNullPtr( reader_ );
	    errmsg_ = uiStrings::sCantReadInp();
	    return false;
	}

	reader_->setSelData( new Seis::RangeSelData(tks_) );
    }

    writer_ = new SeisTrcWriter( *output_ );
    sequentialwriter_ = new SeisSequentialWriter( writer_ );

    return true;
}



bool VolumeConverter::doWork( od_int64, od_int64, int threadidx )
{
    char res = 1;
    SeisTrc trc;

    lock_.lock();
    res = getNewTrace( trc, threadidx );
    lock_.unLock();

    ArrPtrMan<float> owndata = 0;
    SeisTrcValueSeries inputvs( trc, 0 );
    const float* inputptr = inputvs.arr();
    if ( !inputptr )
    {
	owndata = new float[trc.size()];
	inputptr = owndata.ptr();
    }

    const SamplingData<double> sd =
			       getDoubleSamplingData( trc.info().sampling );
    const int trcsz = trc.size();
    TypeSet<double> timevals( trcsz, 0.f );
    for ( int idx=0; idx<trc.size(); idx++ )
	timevals[idx] = sd.atIndex( idx );
    const double* timesamps = timevals.arr();

    while ( res==1 )
    {
	if ( !shouldContinue() )
	    return false;

	if ( owndata )
	{
	    for ( int idx=0; idx<trc.size(); idx++ )
		owndata[idx] = inputvs.value( idx );
	}

	SeisTrc* outputtrc = new SeisTrc( trc );
	float* outptr = (float*) outputtrc->data().getComponent( 0 )->data();
	float* interptr = 0;
	if ( velinpdesc_.type_ != VelocityDesc::Interval )
	{
	    if ( veloutpdesc_ != VelocityDesc::Interval )
	    {
		mTryAlloc(interptr,float[trcsz]);
		if ( !interptr )
		{
		    errmsg_ = tr("Not enough memory");
		    deleteAndNullPtr( outputtrc );
		    return false;
		}
	    }
	    float* targetptr = interptr ? interptr : outptr;
	    if ( (velinpdesc_.type_ == VelocityDesc::Avg &&
		  !::computeVint(inputptr,timesamps,trcsz,targetptr) ) ||
		 (velinpdesc_.type_ == VelocityDesc::RMS &&
		  !computeDix(inputptr,sd,trcsz,targetptr) ) )
	    {
		deleteAndNullPtr( outputtrc );
	    }
	}

	const float* srcptr = interptr ? interptr : inputptr;
	if ( (veloutpdesc_.type_ == VelocityDesc::Avg &&
	      !::computeVavg(srcptr,timesamps,trcsz,outptr) ) ||
	     (veloutpdesc_.type_ == VelocityDesc::RMS &&
	      !computeVrms(srcptr,sd,trcsz,outptr) ) )
	{
	    deleteAndNullPtr( outputtrc );
	}

	delete [] interptr;

	//Process trace
	sequentialwriter_->submitTrace( outputtrc, true );
	addToNrDone( 1 );

	Threads::MutexLocker lock( lock_ );
	res = getNewTrace( trc, threadidx );
    }

    return res==0;
}


char VolumeConverter::getNewTrace( SeisTrc& trc, int threadidx )
{
    if ( !reader_ )
	return 0;

    int res = 2;
    while ( res==2 || !tks_.includes( trc.info().binID() ) )
	res = reader_->get( trc.info() );

    if ( res==1 )
    {
	if ( !reader_->get( trc ) )
	    return -1;

	sequentialwriter_->announceTrace( trc.info().binID() );
	return 1;
    }

    if ( res==-1 )
	errmsg_ = uiStrings::sCantReadInp();

    delete reader_;
    reader_ = 0;

    return mCast( char, res );
}

mStopAllowDeprecatedSection


} // namespace Vel
