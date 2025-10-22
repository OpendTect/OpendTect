/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "progressmeterimpl.h"
#include "timefun.h"
#include "task.h"
#include "od_ostream.h"

static const char progress_symbols[] = ".:=|*#>}].:=|*#>}].:=|*#>}].:=|*#>}]";


ProgressRecorder::ProgressRecorder()
    : forwardto_(nullptr)
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
    // if ( forwardto_ ) forwardto_->reset();
}


#define mSetLock() Threads::Locker lock( lock_ );

#define mImplProgressRecorderStartStopSetFn(nm,memb) \
void ProgressRecorder::nm() \
    { mSetLock(); memb = true; }
mImplProgressRecorderStartStopSetFn(setStarted,isstarted_)
mImplProgressRecorderStartStopSetFn(setFinished,isfinished_)
#define mImplProgressRecorderSetFn(nm,typ,arg,memb) \
void ProgressRecorder::nm( typ arg ) \
    { mSetLock(); memb = arg; if ( forwardto_ ) forwardto_->nm( arg ); }
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

    mDynamicCastGet(TextStreamProgressMeter*,txtfwdto,forwardto_)
    if ( txtfwdto )
	txtfwdto->printMessage( msg );
}


void ProgressRecorder::setForwardTo( ProgressMeter* pm )
{
    if ( pm == this )
	return;

    mSetLock();
    forwardto_ = pm;
}


void ProgressRecorder::skipProgress( bool yn )
{}


void ProgressRecorder::operator++()
{
    mSetLock();
    if (  forwardto_ )
	++(*forwardto_);
    else
	nrdone_++;
}


void ProgressRecorder::setFrom( const Task& t )
{
    mSetLock();
    name_ = t.name();
    nrdone_ = t.nrDone() * t.progressFactor();;
    totalnr_ = t.totalNr() * t.progressFactor();
    message_ = t.uiMessage();
    nrdonetext_ = t.uiNrDoneText();
    if ( nrdone_ > 0 )
	isstarted_ = true;
}


// TextStreamProgressMeter

TextStreamProgressMeter::TextStreamProgressMeter( od_ostream& out,
						  unsigned short rowlen )
    : strm_(out)
    , rowlen_(rowlen)
{
    reset();
}


TextStreamProgressMeter::~TextStreamProgressMeter()
{
    if ( !finished_ )
	setFinished();
}


void TextStreamProgressMeter::setName( const char* newname )
{
    if ( !name_.isEmpty() && name_==newname )
	return;

    name_ = newname;
    reset();
}


void TextStreamProgressMeter::setStarted()
{
    if ( skipprog_ || inited_ )
	return;

    if ( !name_.isEmpty() )
	strm_ <<  "Process: '" << name_.buf() << "'" << od_endl;

    if ( !message_.isEmpty() )
    {
	strm_ << "Started: " << Time::getDateTimeString() << "\n\n";
	strm_ << '\t' << message_ << od_endl;
    }

    oldtime_ = Time::getMilliSeconds();
    finished_ = false;
    inited_ = true;
}


void TextStreamProgressMeter::setFinished()
{
    mSetLock();
    if ( finished_ )
	return;

    annotate( false );
    finished_ = true;

    if ( !name_.isEmpty() )
	strm_ <<  "Process: '" << name_.buf() << "'";

    strm_ << "\nFinished: " << Time::getDateTimeString() << od_endl;

    lock.unlockNow();
    reset();
}


void TextStreamProgressMeter::addProgress( int nr )
{
    if ( skipprog_ )
	return;

    if ( !inited_ )
	setStarted();

    for ( int idx=0; idx<nr; idx++ )
    {
	nrdone_ ++;
	od_uint64 relprogress = nrdone_ - lastannotatednrdone_;
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


void TextStreamProgressMeter::setNrDone( od_int64 nrdone )
{
    mSetLock();
    if ( finished_ || nrdone<=nrdone_ )
	return;

    addProgress( nrdone-nrdone_ );
}


void TextStreamProgressMeter::setTotalNr( od_int64 totalnr )
{
    Threads::Locker lock( lock_ );
    totalnr_ = totalnr;
}


void TextStreamProgressMeter::setMessage( const uiString& message )
{
    if ( message != message_ )
	message_ = message;
}


void TextStreamProgressMeter::printMessage( const uiString& msg )
{
    strm_ << od_newline << msg << od_endl;
}


void TextStreamProgressMeter::operator++()
{
    mSetLock();
    addProgress( 1 );
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
    const od_int64 newtime = Time::getMilliSeconds();
    const od_int64 tdiff = newtime - oldtime_;
    if ( withrate && tdiff > 0 )
    {
	const od_int64 nrdone = nrdone_ - lastannotatednrdone_;
	const od_int64 permsec = (od_int64)(1.e6 * nrdone / tdiff + .5);
	strm_ << " (" << permsec * .001 << "/s)";
    }
    if ( withrate && tdiff>0 && totalnr_>0 )
    {
	const double nrdone = (double) nrdone_ - lastannotatednrdone_;
	const double todo = (double) totalnr_ - nrdone_;
	const od_int64 etasec = mNINT64(tdiff * (todo/nrdone) / 1000.);
	const BufferString eta = Time::getTimeDiffString( etasec, 2 );
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


// SimpleTextStreamProgressMeter

SimpleTextStreamProgressMeter::SimpleTextStreamProgressMeter( od_ostream& out,
							      int repperc )
    : strm_(out)
    , repperc_(repperc)
{
    reset();
}


SimpleTextStreamProgressMeter::~SimpleTextStreamProgressMeter()
{
    if ( !finished_ )
	setFinished();
}


void SimpleTextStreamProgressMeter::setName( const char* newname )
{
    if ( !name_.isEmpty() && name_==newname )
	return;

    name_ = newname;
    reset();
}


void SimpleTextStreamProgressMeter::setStarted()
{
    if ( skipprog_ || inited_ )
	return;

    const bool hasname = !name_.isEmpty();
    const bool hasmessage = !message_.isEmpty();
    if ( hasname )
	strm_ <<  "Process: '" << name_.buf() << "'";

    if ( hasmessage )
    {
	if ( hasname )
	    strm_ << "; ";

	strm_ << "Started at: " << Time::getISODateTimeString(true);
    }

    if ( hasname || hasmessage )
	strm_ << od_endl;

    finished_ = false;
    inited_ = true;
}


void SimpleTextStreamProgressMeter::setFinished()
{
    mSetLock();
    if ( finished_ )
	return;

    addProgress( totalnr_ - nrdone_ );
    finished_ = true;

    const bool hasname = !name_.isEmpty();
    if ( hasname )
	strm_ <<  "Process: '" << name_.buf() << "'; ";

    strm_ << "Finished at: " << Time::getISODateTimeString(true) << od_endl;

    lock.unlockNow();
    reset();
}


void SimpleTextStreamProgressMeter::setNrDone( od_int64 nrdone )
{
    mSetLock();
    if ( finished_ || nrdone<=nrdone_ )
	return;

    addProgress( nrdone-nrdone_ );
}


void SimpleTextStreamProgressMeter::setTotalNr( od_int64 totalnr )
{
    Threads::Locker lock( lock_ );
    totalnr_ = totalnr;
}


void SimpleTextStreamProgressMeter::setMessage( const uiString& message )
{
    if ( message != message_ )
	message_ = message;
}


void SimpleTextStreamProgressMeter::printMessage( const uiString& msg )
{
    strm_ << od_newline << msg << od_endl;
}


void SimpleTextStreamProgressMeter::operator++()
{
    mSetLock();
    addProgress( 1 );
}


void SimpleTextStreamProgressMeter::reset()
{
    mSetLock();
    nrdone_ = 0;
    inited_ = false;
    lastannotatednrdone_ = 0;
}


void SimpleTextStreamProgressMeter::addProgress( int nr )
{
    if ( skipprog_ )
	return;

    if ( !inited_ )
	setStarted();

    const int repint = mNINT32((double)repperc_/100. * totalnr_);
    if ( nrdone_ == totalnr_ && lastannotatednrdone_ < totalnr_ )
    {
	strm_ <<  message_ << "; Progress(%): 100" << od_endl;
    }
    else
    {
	for ( int idx=0; idx<nr; idx++ )
	{
	    if ( !(nrdone_ % repint) )
	    {
		const float perc = float(nrdone_)/float(totalnr_) * 100.f;
		strm_ <<  message_ << "; Progress(%): " << mNINT32(perc)
		      << od_endl;
		lastannotatednrdone_ = nrdone_+1;
	    }

	    nrdone_++;
	}
    }
}
