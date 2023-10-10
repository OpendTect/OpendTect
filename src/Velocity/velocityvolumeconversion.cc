/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocityvolumeconversion.h"

#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistype.h"
#include "seiswrite.h"
#include "timedepthconv.h"
#include "uistrings.h"
#include "veldesc.h"

namespace Vel
{

const char* VolumeConverter::sKeyInput() { return sKey::Input(); }
const char* VolumeConverter::sKeyOutput() { return sKey::Output(); }

VolumeConverter::VolumeConverter( const IOObj& input, const IOObj& output,
				  const TrcKeySampling& ranges,
				  const VelocityDesc& desc, double srd,
				  const UnitOfMeasure* srduom )
    : msg_(tr("Converting velocities"))
    , tks_(ranges)
    , velinpdesc_(*new VelocityDesc)
    , veloutpdesc_(*new VelocityDesc(desc))
    , input_(input)
    , output_(output)
    , srd_(srd)
    , srduom_(srduom)
{
    velinpdesc_.usePar( input_.pars() );
    msg_ = tr("Converting velocities from %1 to %2")
		.arg( toString(velinpdesc_.type_) )
		.arg( toString(veloutpdesc_.type_) );
    setRanges();
}


VolumeConverter::~VolumeConverter()
{
    delete &velinpdesc_;
    delete &veloutpdesc_;
    delete reader_;
    delete writer_;
    delete sequentialwriter_;
}


uiString VolumeConverter::uiMessage() const
{
    return msg_;
}


uiString VolumeConverter::uiNrDoneText() const
{
    return sTrcFinished();
}


od_int64 VolumeConverter::nrIterations() const
{
    return totalnr_;
}


void VolumeConverter::setRanges()
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


bool VolumeConverter::doPrepare( int nrthreads )
{
    msg_ = tr("Converting velocities from %1 to %2")
		.arg( toString(velinpdesc_.type_) )
		.arg( toString(veloutpdesc_.type_) );
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


bool VolumeConverter::doWork( od_int64, od_int64, int threadidx )
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


bool VolumeConverter::doFinish( bool success )
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
    VelocityStretcher::getRange( inpiop, velinpdesc_, true, velrg );
    VelocityStretcher::setRange( velrg, veloutpdesc_, true, outiop );
    VelocityStretcher::getRange( inpiop, velinpdesc_, false, velrg );
    VelocityStretcher::setRange( velrg, veloutpdesc_, false, outiop );
    if ( !IOM().commitChanges(output_) )
    {
	msg_ = uiStrings::phrCannotWrite( tr("velocity information") );
	return false;
    }

    return success;
}


char VolumeConverter::getNewTrace( SeisTrc& trc, int threadidx )
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

} // namespace Vel
