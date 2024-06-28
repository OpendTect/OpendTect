/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "timefun.h"
#include "bufstring.h"
#include "perthreadrepos.h"

#include <QDateTime>
#include <QElapsedTimer>

mUseQtnamespace

namespace Time
{

//Counter

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


// FileTimeSet

FileTimeSet::FileTimeSet()
{
    modtime_.tv_sec = mUdf(std::time_t);
    acctime_.tv_sec = mUdf(std::time_t);
    crtime_.tv_sec = mUdf(std::time_t);
}


FileTimeSet::~FileTimeSet()
{
}


bool FileTimeSet::hasModificationTime() const
{
    return !mIsUdf(modtime_.tv_sec);
}


bool FileTimeSet::hasAccessTime() const
{
    return !mIsUdf(acctime_.tv_sec);
}


bool FileTimeSet::hasCreationTime() const
{
    return !mIsUdf(crtime_.tv_sec);
}


std::timespec FileTimeSet::getModificationTime() const
{
    return modtime_;
}


std::timespec FileTimeSet::getAccessTime() const
{
    return acctime_;
}


std::timespec FileTimeSet::getCreationTime() const
{
    return crtime_;
}


FileTimeSet& FileTimeSet::setModificationTime( const std::timespec& t )
{
    modtime_  = t;
    return *this;
}


FileTimeSet& FileTimeSet::setAccessTime( const std::timespec& t )
{
    acctime_ = t;
    return *this;
}


FileTimeSet& FileTimeSet::setCreationTime( const std::timespec& t )
{
    crtime_ = t;
    return *this;
}


// Global functions

od_int64 getMilliSeconds()
{
    return QDateTime::currentMSecsSinceEpoch();
}


od_int64 passedSince( od_int64 timestamp )
{
    const od_int64 elapsed = timestamp > 0 ? getMilliSeconds() - timestamp : -1;
    return elapsed;
}


const char* defDateTimeFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss"; }
const char* defDateTimeTzFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss, t"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }


static const char* getDateTimeString( const QDateTime& qdtin, const char* fmt,
				      bool local )
{
    mDeclStaticString( ret );
    const QDateTime qdt = local ? qdtin : qdtin.toUTC();
    if ( !fmt || !*fmt )
	ret = qdt.toString( Qt::ISODate );
    else
	ret = qdt.toString( fmt );

    return ret.buf();

}


const char* getISODateTimeString( bool local )
{
    mDeclStaticString( ret );
    const QDateTime qdt = QDateTime::currentDateTime();
    ret.set( getDateTimeString( qdt, nullptr, local ) );
    return ret.buf();
}


const char* getDateTimeString( const char* fmt, bool local )
{
    mDeclStaticString( ret );

    const QDateTime qdt = QDateTime::currentDateTime();
    ret.set( getDateTimeString( qdt, fmt, local ) );
    return ret.buf();
}


const char* getDateTimeString( od_int64 timems, const char* fmt, bool local )
{
    mDeclStaticString( ret );

    const QDateTime qdt = QDateTime::fromMSecsSinceEpoch( timems );
    ret.set( getDateTimeString( qdt, fmt, local ) );
    return ret.buf();
}


const char* getDateTimeString( const std::timespec& time,
			       const char* fmt, bool local )
{
    mDeclStaticString( ret );

    const od_int64 timems = od_int64 (od_int64 (time.tv_sec) * 1000 +
				      (od_int64)time.tv_nsec / 1e6 );
    ret.set( getDateTimeString( timems, fmt, local ) );
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


const char* getTimeDiffString( od_int64 deltasec, int precision )
{
    if ( precision<1 || precision>4 )
	precision = 4;

    mDeclStaticString( ret );
    ret.setEmpty();
    int usedprec = 0;
    const int daysec = 86400, hoursec = 3600, minsec = 60;
    const bool adddays = deltasec>daysec;
    if ( adddays )
    {
	const int days = sCast(int,deltasec/daysec);
	ret.add( days ).add( "d:" );
	deltasec = deltasec%daysec;
	usedprec++;
    }

    const bool addhours = (adddays || deltasec>hoursec) && usedprec<precision;
    if ( addhours )
    {
	const int hours = sCast(int,deltasec/hoursec);
	ret.add( hours ).add( "h:" );
	deltasec = deltasec%hoursec;
	usedprec++;
    }

    const bool addmin = (addhours || deltasec>minsec) && usedprec<precision;
    if ( addmin )
    {
	const int mins = sCast(int,deltasec/minsec);
	ret.add( mins ).add( "m:" );
	deltasec = deltasec%minsec;
	usedprec++;
    }

    const bool addsec = usedprec < precision;
    if ( addsec )
	ret.add( deltasec ).add( "s" );

    return ret;
}

} // namespace Time
