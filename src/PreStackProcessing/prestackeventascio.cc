/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackeventascio.h"

#include "binidvalue.h"
#include "executor.h"
#include "file.h"
#include "od_iostream.h"
#include "tabledef.h"
#include "uistrings.h"
#include "unitofmeasure.h"


namespace PreStack
{

EventExporter::EventExporter( od_ostream& strm, EventManager& events )
    : strm_(strm)
    , events_(events)
    , tks_(true)
    , nrdone_(0)
    , fileidx_(0)
    , locations_(0,false)
    , message_(uiString::emptyString())
{
    events_.ref();
    events_.getLocations( locations_ );
}


EventExporter::~EventExporter()
{ events_.unRef(); }


void EventExporter::setHRange( const TrcKeySampling& hrg )
{
    tks_ = hrg;
}


uiString EventExporter::uiNrDoneText() const
{ return tr("CDP's exported"); }

#define mWrite( var ) strm_ << (var) << '\t';
#define mBatchSize    10000



int EventExporter::nextStep()
{
    bool isatend = false;
    message_ = tr("Reading");

    BinIDValueSet currentbatch( 0, false );
    for ( int idx=0; idx<mBatchSize; idx++ )
    {
	if ( !locations_.next( pos_ ) )
	{
	    isatend = true;
	    break;
	}

	const BinID bid = locations_.getBinID(pos_);
	if ( !tks_.includes( bid ) )
	    continue;

	currentbatch.add( bid );
    }


    PtrMan<Executor> exec = events_.load( currentbatch, false );
    if ( !exec || !exec->execute() )
	return ErrorOccurred();

    message_ = tr("Writing");
    BinIDValueSet::SPos pos;

    while ( currentbatch.next( pos ) )
    {
	const BinID bid = currentbatch.getBinID( pos );
	ConstRefMan<EventSet> eventset = events_.getEvents( bid, false,false );
	if ( !eventset )
	    return ErrorOccurred();

	for ( int eventidx=0; eventidx<eventset->events_.size(); eventidx++ )
	{
	    const Event& event = *eventset->events_[eventidx];
	    if ( event.sz_==0 )
		continue;

	    float inldip = 0, crldip = 0;
	    events_.getDip( BinIDValue(bid,event.pick_[0]), event.horid_,
			    inldip, crldip );

	    for ( int pickidx=0; pickidx<event.sz_; pickidx++ )
	    {
		mWrite( bid.inl() );
		mWrite( bid.crl() );
		mWrite( fileidx_ );
		mWrite( inldip );
		mWrite( crldip );
		mWrite( (int) event.quality_ );
		mWrite( event.offsetazimuth_[pickidx].azimuth() );
		mWrite( event.offsetazimuth_[pickidx].offset() );
		mWrite( event.pick_[pickidx] );
		mWrite( event.pickquality_ ? event.pickquality_[pickidx] : 255);
		strm_ << '\n';
	    }

	    fileidx_++;
	}

	nrdone_++;
    }

    return isatend ? Finished() : MoreToDo();
}



// EventImporter
EventImporter::EventImporter( const char* filenm, const Table::FormatDesc& fd,
			      EventManager& evmgr )
    : evmgr_(evmgr)
    , strm_(*new od_istream(filenm))
    , ascio_(0)
    , event_(0)
    , lasthorid_(-1)
    , message_(uiString::emptyString())
{
    if ( strm_.isOK() )
	ascio_ = new EventAscIO( fd, strm_ );
    totalnr_ = File::getFileSize( filenm );
    evmgr_.ref();
    message_ = ascio_
	? tr("Importing")
	: uiStrings::phrCannotRead( uiStrings::sInputData() );
}


EventImporter::~EventImporter()
{ evmgr_.unRef(); delete &strm_; delete ascio_; }


uiString EventImporter::uiNrDoneText() const
{ return tr("Nr bytes read"); }


od_int64 EventImporter::nrDone() const
{
    return strm_.isOK() ? strm_.position() : 0;
}


int EventImporter::nextStep()
{
    if ( !ascio_ )
	return ErrorOccurred();

    BinID bid;
    int horid = -1;
    float offset, zval;
    const int ret = ascio_->getNextLine( bid, horid, offset, zval );
    if ( ret < 0 )
    { message_ = tr("Error during read"); return ErrorOccurred(); }

    if ( bid != lastbid_ || horid != lasthorid_ )
    {
	if ( event_ )
	{
	    EventSet* evset = evmgr_.getEvents( lastbid_, false, true );
	    if ( evset )
	    {
		evset->events_ += event_;
		evset->ischanged_ = true;
	    }
	}

	event_ = new Event( 0, 0 );
    }

    event_->addPick();
    const int idx = event_->sz_ - 1;
    event_->pick_[idx] = zval;
    event_->offsetazimuth_[idx].setOffset( offset );
    lastbid_ = bid;
    lasthorid_ = horid;
    return ret ? MoreToDo() : Finished();
}


// EventAscIO

EventAscIO::EventAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , udfval_(mUdf(float))
    , strm_(strm)
{
}


EventAscIO::~EventAscIO()
{
}


Table::FormatDesc* EventAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PreStack Event" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    createDescBody( fd );
    return fd;
}


void EventAscIO::createDescBody( Table::FormatDesc* fd )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Event ID (optional)",
					     IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Offset", FloatInpSpec(),
					     Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


void EventAscIO::updateDesc( Table::FormatDesc& fd )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool EventAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


int EventAscIO::getNextLine( BinID& bid, int& horid,
			     float& offset, float& zval )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }

    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 )
	return ret;

    bid = getBinID( 0, 1 );

    horid = getIntValue( 2, mUdf(od_int16) );
    offset = getFValue( 3, udfval_ );
    zval = getFValue( 4, udfval_ );
    return ret;
}


} // namespace PreStack
