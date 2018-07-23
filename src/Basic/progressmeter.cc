/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "progressmeterimpl.h"
#include "timefun.h"
#include "task.h"
#include "od_ostream.h"

static const char progress_symbols[] = ".:=|*#>}].:=|*#>}].:=|*#>}].:=|*#>}]";


ProgressRecorder::ProgressRecorder()
    : forwardto_(0)
    , skipprog_(false)
    , lock_(*new Threads::Lock)
{
    reset();
}


ProgressRecorder::~ProgressRecorder()
{
    if ( !isfinished_ )
	setFinished();
    delete &lock_;
}


void ProgressRecorder::reset()
{
    nrdone_ = 0;
    totalnr_ = -1;
    isstarted_ = isfinished_ = false;
    message_.setEmpty(); nrdonetext_.setEmpty();
    // if ( forwardto_ && forwardto_ != this ) forwardto_->reset();
}


#define mSetLock() Threads::Locker lock( lock_ );

#define mImplProgressRecorderStartStopSetFn(nm,memb) \
void ProgressRecorder::nm() \
    { mSetLock(); memb = true; \
      if ( forwardto_ && !skipprog_ && forwardto_ != this ) \
	forwardto_->nm(); }
mImplProgressRecorderStartStopSetFn(setStarted,isstarted_)
mImplProgressRecorderStartStopSetFn(setFinished,isfinished_)
#define mImplProgressRecorderSetFn(nm,typ,arg,memb) \
void ProgressRecorder::nm( typ arg ) \
    { mSetLock(); memb = arg; \
      if ( forwardto_ && forwardto_ != this ) forwardto_->nm( arg ); }
mImplProgressRecorderSetFn(setName,const char*,newnm,name_)
mImplProgressRecorderSetFn(setTotalNr,od_int64,tnr,totalnr_)
mImplProgressRecorderSetFn(setNrDone,od_int64,nr,nrdone_)
mImplProgressRecorderSetFn(setNrDoneText,const uiString&,txt,nrdonetext_)

#define mImplProgressRecorderStartStopGetFn(typ,nm,memb) \
typ ProgressRecorder::nm() const { mSetLock(); return memb; }
mImplProgressRecorderStartStopGetFn(const char*,name,name_)
mImplProgressRecorderStartStopGetFn(od_int64,nrDone,nrdone_)
mImplProgressRecorderStartStopGetFn(od_int64,totalNr,totalnr_)
mImplProgressRecorderStartStopGetFn(uiString,message,message_)
mImplProgressRecorderStartStopGetFn(uiString,nrDoneText,nrdonetext_)
mImplProgressRecorderStartStopGetFn(bool,isStarted,isstarted_)
mImplProgressRecorderStartStopGetFn(bool,isFinished,isfinished_)
mImplProgressRecorderStartStopGetFn(ProgressMeter*,forwardTo,forwardto_)

void ProgressRecorder::setMessage( const uiString& msg )
{
    message_ = msg;
}


void ProgressRecorder::setMessage( const uiString& msg, bool doprint )
{
    setMessage( msg );
    if ( !doprint )
        return;

    if ( forwardto_ )
	forwardto_->printMessage( msg );
}


void ProgressRecorder::setForwardTo( ProgressMeter* pm )
{
    mSetLock();
    forwardto_ = pm;
}

void ProgressRecorder::skipProgress( bool yn )
{
    mSetLock();
    skipprog_ = yn;
}


void ProgressRecorder::operator++()
{
    mSetLock();
    if ( forwardto_ )
	++(*forwardto_);
    else
	nrdone_++;
}


void ProgressRecorder::setFrom( const Task& t )
{
    mSetLock();
    name_ = t.name();
    nrdone_ = t.nrDone();
    totalnr_ = t.totalNr();
    message_ = t.message();
    nrdonetext_ = t.nrDoneText();
    if ( nrdone_ > 0 )
	isstarted_ = true;
}



TextStreamProgressMeter::TextStreamProgressMeter( od_ostream& out,
						  od_uint16 rowlen )
    : strm_(out)
    , rowlen_(rowlen)
    , finished_(true)
    , totalnr_(0)
    , lock_(*new Threads::Lock)
{
    skipprog_ = false;
    reset();
}


TextStreamProgressMeter::~TextStreamProgressMeter()
{
    if ( !finished_ )
	setFinished();
    delete &lock_;
}


void TextStreamProgressMeter::setTotalNr( od_int64 t )
{
    mSetLock();
    totalnr_ = t;
}


void TextStreamProgressMeter::setFinished()
{
    mSetLock();
    if ( finished_ )
	return;

    annotate(false);
    finished_ = true;

    if ( name_.isEmpty() )
	strm_ << od_newline;
    else
	strm_ <<  "Process: '" << name_.buf() << "'\n";

    strm_ << "Finished: "  << Time::getUsrDateTimeString() << od_endl;

    lock.unlockNow();
    reset();
}


void TextStreamProgressMeter::reset()
{
    mSetLock();
    nrdone_ = 0;
    oldtime_ = Time::getMilliSeconds();
    inited_ = false;
    nrdoneperchar_ = 1; distcharidx_ = 0;
    lastannotatednrdone_ = 0;
    nrdotsonline_ = 0;
}


void TextStreamProgressMeter::setStarted()
{
    if ( skipprog_ ) return;

    if ( !inited_ )
    {
	if ( !name_.isEmpty() ) strm_ <<  "Process: '" << name_.buf() << "'\n";
	if ( !message_.isEmpty() )
	{
	    strm_ << "Started: " << Time::getUsrDateTimeString() << "\n\n";
	    strm_ << '\t' << toString( message_ ) << od_endl;
	}

        oldtime_ = Time::getMilliSeconds();
	finished_ = false;
	inited_ = true;
    }
}


void TextStreamProgressMeter::addProgress( int nr )
{
    if ( skipprog_ ) return;

    if ( !inited_ )
	setStarted();

    for ( int idx=0; idx<nr; idx++ )
    {
	nrdone_ ++;
	od_int64 relprogress = nrdone_ - lastannotatednrdone_;
	if ( !(relprogress % nrdoneperchar_) )
	{
	    strm_ << (relprogress%(10*nrdoneperchar_)
		    ? progress_symbols[distcharidx_]
		    : progress_symbols[distcharidx_+1]);
	    strm_.flush();
	    nrdotsonline_++;
	}

	if ( nrdotsonline_==rowlen_ )
	    annotate(true);
    }
}


void TextStreamProgressMeter::operator++()
{
    mSetLock();
    addProgress( 1 );
}


void TextStreamProgressMeter::setNrDone( od_int64 nrdone )
{
    mSetLock();
    if ( finished_ || nrdone<=nrdone_ )
	return;

    addProgress( (int)(nrdone-nrdone_) );
}


void TextStreamProgressMeter::setMessage( const uiString& message )
{
    message_ = message;
}


void TextStreamProgressMeter::printMessage( const uiString& msg )
{
    strm_ << od_newline << toString( msg ) << od_endl;
}


void TextStreamProgressMeter::setName( const char* newname )
{
    if ( !name_.isEmpty() && name_==newname )
	return;

    name_ = newname;
    reset();
}


void TextStreamProgressMeter::annotate( bool withrate )
{
    // Show numbers
    strm_ << ' ';
    float percentage = 0;
    if ( totalnr_>0 )
    {
	percentage = ((float) nrdone_)/totalnr_;
	strm_ << mNINT32(percentage*100) << "%";
    }
    else
	strm_ << nrdone_;


    // Show rate
    int newtime = Time::getMilliSeconds();
    int tdiff = newtime - oldtime_;
    if ( withrate && tdiff > 0 )
    {
	od_int64 nrdone = nrdone_ - lastannotatednrdone_;
	od_int64 permsec = (od_int64)(1.e6 * nrdone / tdiff + .5);
	strm_ << " (" << permsec * .001 << "/s)";
    }
    if ( withrate && tdiff>0 && totalnr_>0 )
    {
	const float nrdone = (float) nrdone_ - lastannotatednrdone_;
	const float todo = (float) totalnr_ - nrdone_;
	od_int64 etasec = mNINT64(tdiff * (todo/nrdone) / 1000.f);
	BufferString eta;
	if ( etasec > 3600 )
	{
	    const int hours = (int) etasec/3600;
	    eta.add(hours).add("h:");
	    etasec = etasec%3600;
	}
	if ( etasec > 60 )
	{
	    const int mins = (int) etasec/60;
	    eta.add(mins).add("m:");
	    etasec = etasec%60;
	}

	eta.add(etasec).add("s");

	strm_ << " (" << eta << ")";
    }
    strm_ << od_endl;

    lastannotatednrdone_ = nrdone_;
    oldtime_ = newtime;
    nrdotsonline_ = 0;

    // Adjust display speed if necessary
    if ( tdiff > -1 && tdiff < 5000 )
    {
	distcharidx_++;
	nrdoneperchar_ *= 10;
    }
    else if ( tdiff > 60000 )
    {
	if ( distcharidx_ )
	{
	    distcharidx_--;
	    nrdoneperchar_ /= 10;
	}
    }
}
