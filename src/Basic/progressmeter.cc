/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id$";

#include "progressmeter.h"
#include "timefun.h"
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include "task.h"


static const char dispchars_[] = ".:=|*#>}ABCDEFGHIJKLMNOPQRSTUVWXYZ";


TextStreamProgressMeter::TextStreamProgressMeter( std::ostream& out,
						  unsigned short rowlen )
    : strm_(out)
    , rowlen_(rowlen)
    , finished_(true)
    , totalnr_(0)
{ reset(); }


TextStreamProgressMeter::~TextStreamProgressMeter()
{
    if ( !finished_ ) setFinished();
}

/*
void TextStreamProgressMeter::setTask( const Task& task )
{
    strm_ <<  "Process: '" << task.name() << "'\n";
    strm_ << "Started: " << Time::getDateTimeString() << "\n\n";
    strm_ << '\t' << task.message() << '\n';
    reset();
}

*/


void TextStreamProgressMeter::setFinished()
{
    Threads::MutexLocker lock( lock_ );
    if ( finished_ )
	return;

    annotate(false);
    finished_ = true;

    strm_ << "\nFinished: "  << Time::getDateTimeString() << std::endl;
    lock.unLock();
    reset();
}


void TextStreamProgressMeter::reset()
{
    Threads::MutexLocker lock( lock_ );
    nrdone_ = 0;
    oldtime_ = Time::getMilliSeconds();
    inited_ = false;
    nrdoneperchar_ = 1; distcharidx_ = 0;
    lastannotatednrdone_ = 0;
    nrdotsonline_ = 0;
}


void TextStreamProgressMeter::setStarted()
{
    if ( !inited_ )
    {
	if ( !name_.isEmpty() ) strm_ <<  "Process: '" << name_.buf() << "'\n";
	strm_ << "Started: " << Time::getDateTimeString() << "\n\n";
	if ( !message_.isEmpty() ) strm_ << '\t' << message_.buf() << '\n';
        oldtime_ = Time::getMilliSeconds();
	strm_.flush();
	finished_ = false;
	inited_ = true;
    }
}


void TextStreamProgressMeter::addProgress( int nr )
{
    if ( !inited_ )
	setStarted();

    for ( int idx=0; idx<nr; idx++ )
    {
	nrdone_ ++;
	unsigned long relprogress = nrdone_ - lastannotatednrdone_;
	if ( !(relprogress % nrdoneperchar_) )
	{
	    strm_ << (relprogress%(10*nrdoneperchar_)
		    ? dispchars_[distcharidx_]
		    : dispchars_[distcharidx_+1]);
	    strm_.flush();
	    nrdotsonline_++;
	}

	if ( nrdotsonline_==rowlen_ )
	    annotate(true);
    }
}


void TextStreamProgressMeter::operator++()
{
    Threads::MutexLocker lock( lock_ );
    addProgress( 1 );
}
	

void TextStreamProgressMeter::setNrDone( od_int64 nrdone )
{
    Threads::MutexLocker lock( lock_ );
    if ( nrdone<=nrdone_ )
    	return;

    addProgress( nrdone-nrdone_ );
}


void TextStreamProgressMeter::setMessage( const char* message )
{
    if ( message_==message )
	return;

    message_ = message;
}


void TextStreamProgressMeter::setName( const char* newname )
{
    if ( name_==newname )
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
	float nrdone;
	if ( totalnr_>0 )
 	{
	    const float lastpercentage = ((float)lastannotatednrdone_)/totalnr_;
	    nrdone = (percentage-lastpercentage)*100;
	}
 	else
 	    nrdone = nrdone_ - lastannotatednrdone_;

	od_int64 permsec = (od_int64)(1.e6 * nrdone / tdiff + .5);
	strm_ << " (" << permsec * .001;
	if ( totalnr_>0 ) strm_ << "%";
	strm_ << "/s)";

    }
    strm_ << std::endl;

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
