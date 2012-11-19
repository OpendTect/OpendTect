/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "timefun.h"
#include "bufstring.h"
#include "staticstring.h"

#include <QDateTime>
#include <QTime>

mUseQtnamespace

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


const char* defDateTimeFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }

const char* getDateTimeString( const char* fmt, bool local )
{
    static StaticStringManager stm;
    BufferString& datetimestr = stm.getString();
    QDateTime qdt = QDateTime::currentDateTime();
    if ( !local ) qdt = qdt.toUTC();
    datetimestr = qdt.toString( fmt ).toLatin1().constData();
    return datetimestr.buf();
}

const char* getDateString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

const char* getTimeString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

bool isEarlier(const char* first, const char* second, const char* fmt )
{
    QString fmtstr( fmt );
    QDateTime qdt1 = QDateTime::fromString( first, fmtstr );
    QDateTime qdt2 = QDateTime::fromString( second, fmtstr );
    return qdt1 < qdt2;
}

} // namespace Time
