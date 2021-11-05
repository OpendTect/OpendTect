/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/


#include "timefun.h"
#include "bufstring.h"
#include "perthreadrepos.h"

#include <QDateTime>
#include <QElapsedTimer>

mUseQtnamespace

namespace Time
{

Counter::Counter()
    : qelapstimer_(new QElapsedTimer)
{}

Counter::~Counter()
{
    delete qelapstimer_;
}

void Counter::start()
{
    qelapstimer_->start();
}

int Counter::restart()
{
    return qelapstimer_->restart();
}

int Counter::elapsed() const
{
    return qelapstimer_->elapsed();
}


int getMilliSeconds()
{
    QTime daystart(0,0,0,0);
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
const char* defDateTimeTzFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss, t"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }


const char* getISODateTimeString( bool local )
{
    mDeclStaticString( ret );
    const QDateTime qdt = local ? QDateTime::currentDateTime()
				: QDateTime::currentDateTimeUtc();
    ret = qdt.toString( Qt::ISODate );
    return ret.buf();
}


const char* getDateTimeString( const char* fmt, bool local )
{
    mDeclStaticString( ret );

    QDateTime qdt = QDateTime::currentDateTime();
    if ( !local )
	qdt = qdt.toUTC();

    if ( !fmt || !*fmt )
	ret = qdt.toString( Qt::ISODate );
    else
	ret = qdt.toString( fmt );

    return ret.buf();
}


const char* getDateString( const char* fmt, bool local )
{
    if ( fmt && *fmt )
	return getDateTimeString( fmt, local );

    mDeclStaticString( ret );
    QDate date = QDateTime::currentDateTime().date();
    ret = date.toString( Qt::ISODate );
    return ret;
}


const char* getTimeString( const char* fmt, bool local )
{
    if ( fmt && *fmt )
	return getDateTimeString( fmt, local );

    mDeclStaticString( ret );
    QTime time = QDateTime::currentDateTime().time();
    ret = time.toString( Qt::ISODate );
    return ret;
}


const char* getLocalDateTimeFromString( const char* str )
{
    mDeclStaticString( ret );
    QDateTime qdt = QDateTime::fromString( str, Qt::ISODate );
    if ( qdt.isValid() )
    {
	qdt = qdt.toLocalTime();
	ret = qdt.toString( defDateTimeFmt() );
	return ret.buf();
    }

    qdt = QDateTime::fromString( str );
    if ( qdt.isValid() )
    {
	ret = qdt.toString( defDateTimeFmt() );
	return ret.buf();
    }

    ret = str;
    return ret.buf();
}


bool isEarlier(const char* first, const char* second, const char* fmt )
{
    QDateTime qdt1, qdt2;
    if ( !fmt || !*fmt )
    {
	qdt1 = QDateTime::fromString( first, Qt::ISODate );
	qdt2 = QDateTime::fromString( second, Qt::ISODate );
    }
    else
    {
	qdt1 = QDateTime::fromString( first, fmt );
	qdt2 = QDateTime::fromString( second, fmt );
    }
    return qdt1 < qdt2;
}


const char* getTimeString( od_int64 sec, int precision )
{
    if ( precision<1 || precision>4 )
	precision = 4;

    mDeclStaticString( ret );
    ret.setEmpty();
    int usedprec = 0;
    const int daysec = 86400, hoursec = 3600, minsec = 60;
    const bool adddays = sec>daysec;
    if ( adddays )
    {
	const int days = sCast(int,sec/daysec);
	ret.add(days).add("d:");
	sec = sec%daysec;
	usedprec++;
    }

    const bool addhours = (adddays || sec>hoursec) && usedprec<precision;
    if ( addhours )
    {
	const int hours = sCast(int,sec/hoursec);
	ret.add(hours).add("h:");
	sec = sec%hoursec;
	usedprec++;
    }

    const bool addmin = (addhours || sec>minsec) && usedprec<precision;
    if ( addmin )
    {
	const int mins = sCast(int,sec/minsec);
	ret.add(mins).add("m:");
	sec = sec%minsec;
	usedprec++;
    }

    const bool addsec = usedprec < precision;
    if ( addsec )
	ret.add(sec).add("s");

    return ret;
}


const char* getUsrStringFromISO( const char* isostr,
				 const char* fmt, bool local )
{
    mDeclStaticString( ret );
    QDateTime qdt = QDateTime::fromString( QString(isostr), Qt::ISODate );
    if ( local )
	qdt = qdt.toLocalTime();
    ret = qdt.toString( fmt );
    return ret.buf();
}

} // namespace Time
