/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID = "$Id: timefun.cc,v 1.19 2010-03-03 07:28:10 cvsranojay Exp $";

#include "timefun.h"
#include "bufstring.h"

#include <QDateTime>
#include <QTime>

namespace Time
{

Counter::Counter()
    : qtime_(*new QTime)
{}

Counter::~Counter()
{ delete &qtime_; }

void Counter::start()
{ qtime_.start(); }

int Counter::restart()
{ return qtime_.restart(); }

int Counter::elapsed() const
{ return qtime_.elapsed(); }


int getMilliSeconds()
{
    QTime daystart;
    return daystart.msecsTo( QTime::currentTime() );
}


int passedSince( int timestamp )
{
    int elapsed = timestamp > 0 ? getMilliSeconds() - timestamp : -1;
    if ( elapsed < 0 && timestamp > 0 && (elapsed + 86486400 < 86400000) )
	elapsed += 86486400;

    return elapsed;
}


const char* defDateTimeFmt()	{ return "ddd MMM dd yyyy, hh:mm:ss"; }
const char* defDateFmt()	{ return "ddd MMM dd yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }

const char* getDateTimeString( const char* fmt, bool local )
{
    static BufferString datetimestr;
    QDateTime qdt = QDateTime::currentDateTime();
    if ( !local ) qdt = qdt.toUTC();
    datetimestr = qdt.toString( fmt ).toAscii().constData();
    return datetimestr.buf();
}

const char* getDateString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

const char* getTimeString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

}
