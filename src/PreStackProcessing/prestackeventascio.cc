/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "prestackeventascio.h"
#include "executor.h"
#include "file.h"
#include "strmdata.h"
#include "strmoper.h"
#include "strmprov.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace PreStack
{

EventExporter::EventExporter( std::ostream& strm, EventManager& events )
    : strm_( strm )
    , events_( events )
    , hrg_( true )
    , nrdone_( 0 )
    , fileidx_( 0 )
    , locations_( 0, false )
    , message_( "" )
{
    events_.ref();
    events_.getLocations( locations_ );
}


EventExporter::~EventExporter()
{ events_.unRef(); }


void EventExporter::setHRange( const HorSampling& hrg )
{
    hrg_ = hrg;
}


const char* EventExporter::nrDoneText() const
{ return "CDP's exported"; }

#define mWrite( var ) strm_ << (var) << '\t';
#define mBatchSize    10000



int EventExporter::nextStep()
{
    bool isatend = false;
    message_ = "Reading";

    BinIDValueSet currentbatch( 0, false );
    for ( int idx=0; idx<mBatchSize; idx++ )
    {
	if ( !locations_.next( pos_ ) )
	{
	    isatend = true;
	    break;
	}

	const BinID bid = locations_.getBinID(pos_);
	if ( !hrg_.includes( bid ) )
	    continue;

	currentbatch.add( bid );
    }


    PtrMan<Executor> exec = events_.load( currentbatch, false );
    if ( !exec || !exec->execute() )
	return ErrorOccurred();

    message_ = "Writing";
    BinIDValueSet::Pos pos;

    while ( currentbatch.next( pos ) )
    {
	const BinID bid = currentbatch.getBinID( pos );
	RefMan<const EventSet> eventset = events_.getEvents( bid, false,false );
	if ( !eventset )
	    return ErrorOccurred();

	for ( int eventidx=0; eventidx<eventset->events_.size(); eventidx++ )
	{
	    const Event& event = *eventset->events_[eventidx];
	    if ( event.sz_==0 )
		continue;

	    float inldip = 0, crldip = 0;
	    events_.getDip( BinIDValue( bid, event.pick_[0]), event.horid_,
			    inldip, crldip );

	    for ( int pickidx=0; pickidx<event.sz_; pickidx++ )
	    {
		mWrite( bid.inl );
		mWrite( bid.crl );
		mWrite( fileidx_ );
		mWrite( inldip );
		mWrite( crldip );
		mWrite( (int) event.quality_ );
		mWrite( event.offsetazimuth_[pickidx].azimuth() );
		mWrite( event.offsetazimuth_[pickidx].offset() );
		mWrite( event.pick_[pickidx] );
		mWrite( event.pickquality_ ? event.pickquality_[pickidx] : 255 );
		strm_ << '\n';
	    }

	    fileidx_++;
	}

	nrdone_++;
    }

    return isatend ? Finished() : MoreToDo();
}


EventImporter::EventImporter( const char* filenm, const Table::FormatDesc& fd,
			      EventManager& evmgr )
    : evmgr_(evmgr)
    , sd_(*new StreamData(StreamProvider(filenm).makeIStream(false,false)))
    , ascio_(0)
    , event_(0)
    , lasthorid_(-1)
    , message_("")
{
    if ( sd_.istrm )
	ascio_ = new EventAscIO( fd, *sd_.istrm );

    totalnr_ = File::getFileSize( filenm );
    evmgr_.ref();
}


EventImporter::~EventImporter()
{ evmgr_.unRef(); delete &sd_; delete ascio_; }


const char* EventImporter::nrDoneText() const
{ return "Nr bytes read"; }


od_int64 EventImporter::nrDone() const
{
    if ( !sd_.istrm ) return 0;
    return StrmOper::tell( *sd_.istrm );
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
	return ErrorOccurred();

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



Table::FormatDesc* EventAscIO::getDesc() 
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PreStack Event" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
	    		StringInpSpec(sKey::FloatUdf), Table::Required );
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
	
	udfval_ = getfValue( 0 );
	isxy_ = isXY();
	finishedreadingheader_ = true;
    }

    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 )
	return ret;

    Coord pos( getdValue(0,udfval_), getdValue(1,udfval_) );
    bid = isxy_ ? SI().transform( pos ) : BinID( mNINT32(pos.x), mNINT32(pos.y) );
    horid = getIntValue( 2, mUdf(od_int16) );
    offset = getfValue( 3, udfval_ );
    zval = getfValue( 4, udfval_ );
    return ret;
}


} // namespace PreStack
