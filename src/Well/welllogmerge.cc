/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		May 2020
________________________________________________________________________

-*/

#include "welllogmerge.h"

#include "interpol1d.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellman.h"
#include "welltrack.h"
#include "wellwriter.h"


mDefineEnumUtils( Well::LogMerger, OverlapAction, "Overlap action" )
{ Well::LogMerger::sKeyAverage(), Well::LogMerger::sKeyOneLog(), 0 };
 template <>
 void EnumDefImpl<Well::LogMerger::OverlapAction>::init()
 {
     uistrings_ += ::toUiString( "Use average" );
     uistrings_ += ::toUiString( "Use one log" );
 }


Well::LogMerger::LogMerger( const MultiID& wllid,
			    const BufferStringSet& chosenlognms,
			    Well::Log& outputlog )
    : Executor("Merging Logs")
    , outputlog_(outputlog)
{
    wd_ = Well::MGR().get( wllid );
    for ( int idx=0; idx<chosenlognms.size(); idx++ )
    {
	if ( wd_->logs().isPresent(chosenlognms.get(idx)) )
	    lognms_.add( chosenlognms.get(idx) );
    }

}


Well::LogMerger::~LogMerger()
{
    delete logsamp_;
}


od_int64 Well::LogMerger::totalNr() const
{
    return extrapolate_ ? 3 : 2;
}


void Well::LogMerger::setSamplingDist( float zsampling )
{
    zsampling_ = zsampling;
}


void Well::LogMerger::setOverlapAction( Well::LogMerger::OverlapAction action )
{
    overlapaction_ = action;
}


void Well::LogMerger::setDoInterpolation( bool interpolate )
{
    interpolate_ = interpolate;
}


void Well::LogMerger::setDoExtrapolation( bool extrapolate )
{
    extrapolate_ = extrapolate;
}


int Well::LogMerger::nextStep()
{
    if ( nrDone() == 0 )
	return prepare();

    if ( nrDone() == 1 )
	return merge();

    if ( nrDone() == 2 )
	return extrapolateOutLog();

    return ErrorOccurred();
}


bool Well::LogMerger::goImpl( od_ostream* strm, bool first, bool last,
			      int delay )
{
    if ( !wd_ )
    {
	msg_ = tr("Failed to read well %1").arg(wd_->name());
	return false;
    }

    nrdone_ = 0;
    const bool success = Executor::goImpl( strm, first, last, delay );
    deleteAndZeroPtr( logsamp_ );

    return success;
}


int Well::LogMerger::prepare()
{
    nrdone_++;
    outputlog_.setEmpty();
    outputlog_.setUnitMeasLabel(wd_->logs()
				.getLog(lognms_.get(0))->unitMeasLabel());
    Interval<float> dahrg( mUdf(float), mUdf(float) );
    for ( int idx = 0; idx<lognms_.size(); idx++ )
    {
	Interval<float> drg = wd_->logs().getLog(lognms_.get(idx))->dahRange();
	if ( mIsUdf(dahrg.start) || dahrg.start > drg.start )
	    dahrg.start = drg.start;

	if ( mIsUdf(dahrg.stop) || dahrg.stop < drg.stop )
	    dahrg.stop = drg.stop;
    }
    Interval<float> zrg;
    zrg.start = wd_->track().getPos(dahrg.start).z;
    zrg.stop = wd_->track().getPos(dahrg.stop).z;
    zrg.limitTo( wd_->track().zRange() );

    if ( interpolate_ )
	logsamp_ = new Well::LogSampler( *wd_, zrg, false, zsampling_, false,
					 Stats::UseAvg, lognms_ );
    else
	logsamp_ = new Well::LogSampler( *wd_, zrg, false, zsampling_, false,
					 Stats::TakeNearest, lognms_ );

    if ( !logsamp_->execute() )
    {
	msg_ = tr("Could not sample logs in well %1").arg(wd_->name());
	return ErrorOccurred();
    }

    return MoreToDo();
}


int Well::LogMerger::merge()
{
    nrdone_++;
    const int nrsamps = logsamp_->nrZSamples();
    if ( nrsamps == 0 )
    {
	msg_ = tr("Could not sample logs in well %1").arg(wd_->name());
	return ErrorOccurred();
    }

    TypeSet<float> vals( nrsamps, 0 );
    for ( int idx=0; idx<nrsamps-1; idx++ )
    {
	int nrlogs=0;
	float totalval = 0;
	for ( int idl=0; idl<lognms_.size(); idl++ )
	{
	    float value = logsamp_->getLogVal( lognms_.get(idl), idx );
	    if ( mIsUdf(value) )
		continue;

	    totalval+=value;
	    nrlogs++;
	    if ( overlapaction_ == Well::LogMerger::UseOneLog )
		break;
	}

	if ( nrlogs )
	    vals[idx] = totalval/nrlogs;

	outputlog_.insertAtDah( logsamp_->getDah(idx), vals[idx] );
    }

    wd_->logs().add( &outputlog_ );
    deleteAndZeroPtr( logsamp_ );
    return nrDone() >= totalNr() ? Finished() : MoreToDo();
}


int Well::LogMerger::extrapolateOutLog()
{
    nrdone_++;
    Interval<float> dahwelltrckrg = wd_->track().dahRange();
    StepInterval<float> trcksamprg( dahwelltrckrg.start, dahwelltrckrg.stop,
				    zsampling_ );
    const int trcknrsamps = trcksamprg.nrSteps() + 1;
    float extrapolvaltop = outputlog_.value(0);
    float extrapolvalbot = outputlog_.value(outputlog_.size()-1);
    Interval<float> dahrg = outputlog_.dahRange();
    for ( int idt=0; idt<trcknrsamps-1; idt++  )
    {
	const float dah = trcksamprg.atIndex(idt);
	if ( dahrg.includes(dah, true) )
	    continue;
	else if ( dah < dahrg.start )
	    outputlog_.insertAtDah( dah, extrapolvaltop );
	else if ( dah > dahrg.stop )
	    outputlog_.insertAtDah( dah, extrapolvalbot );
    }

    return Finished();
}
