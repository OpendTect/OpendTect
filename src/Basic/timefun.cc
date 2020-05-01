/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/


#include "timefun.h"
#include "bufstring.h"
#include "staticstring.h"
#include "file.h"

#ifndef OD_NO_QT
# include <QDateTime>
# include <QElapsedTimer>
#endif

mUseQtnamespace

namespace Time
{

Counter::Counter()
#ifndef OD_NO_QT
    : qelapstimer_(new QElapsedTimer)
#endif
{}

Counter::~Counter()
{
#ifndef OD_NO_QT
    delete qelapstimer_;
#endif
}

void Counter::start()
{
#ifndef OD_NO_QT
    qelapstimer_->start();
#endif
}

int Counter::restart()
{
#ifndef OD_NO_QT
    return qelapstimer_->restart();
#else
    return mUdf(int);
#endif
}

int Counter::elapsed() const
{
#ifndef OD_NO_QT
    return qelapstimer_->elapsed();
#else
    return mUdf(int);
#endif
}


int getMilliSeconds()
{
#ifndef OD_NO_QT
    QTime daystart(0,0,0,0);
    return daystart.msecsTo( QTime::currentTime() );
#else
    return mUdf(int);
#endif
}


int passedSince( int timestamp )
{
    int elapsed = timestamp > 0 ? getMilliSeconds() - timestamp : -1;
    if ( elapsed < 0 && timestamp > 0 && (elapsed + 86486400 < 86400000) )
	elapsed += 86486400;

    return elapsed;
}


od_int64 getFileTimeInSeconds()
{
#ifndef OD_NO_QT
    return QDateTime::currentDateTime().toTime_t();
#else
    return mUdf(od_int64);
#endif
}


const char* getISOUTCDateTimeString()
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    QDateTime qdt = QDateTime::currentDateTimeUtc();
    ret = qdt.toString( Qt::ISODate );
#endif

    return ret.buf();
}


int compareISOUTCDateTimeStrings( const char* first, const char* second )
{
#ifdef OD_NO_QT
    return FixedString(first) == second ? 0 : -1;
#else
    const QDateTime qdt1 = QDateTime::fromString( first, Qt::ISODate );
    const QDateTime qdt2 = QDateTime::fromString( second, Qt::ISODate );
    if ( qdt1 == qdt2 )
	return 0;
    return qdt1 < qdt2 ? -1 : 1;
#endif
}


const char* defDateTimeFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }

const char* getUsrDateTimeStringFromISOUTC( const char* isostr, const char* fmt,
					 bool local )
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    QDateTime qdt = QDateTime::fromString( QString(isostr), Qt::ISODate );
    if ( local )
	qdt = qdt.toLocalTime();
    ret = qdt.toString( fmt );
#endif

    return ret.buf();
}

const char* getUsrDateStringFromISOUTC( const char* isostr, const char* fmt,
					bool local )
{ return getUsrDateTimeStringFromISOUTC( isostr, fmt, local ); }

const char* getUsrTimeStringFromISOUTC( const char* isostr, const char* fmt,
					bool local )
{ return getUsrDateTimeStringFromISOUTC( isostr, fmt, local ); }


const char* getUsrDateTimeString( const char* fmt, bool local )
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    const QDateTime qdt = local ? QDateTime::currentDateTime()
				: QDateTime::currentDateTimeUtc();
    ret = qdt.toString( fmt );
#endif

    return ret.buf();
}

const char* getUsrDateString( const char* fmt, bool local )
{ return getUsrDateTimeString( fmt, local ); }

const char* getUsrTimeString( const char* fmt, bool local )
{ return getUsrDateTimeString( fmt, local ); }


const char* getUsrFileDateTime( const char* fnm, bool modif,
				const char* fmt, bool local )
{
    const BufferString isostr( modif ? File::timeLastModified(fnm)
				     : File::timeCreated(fnm) );
    return getUsrDateTimeStringFromISOUTC( isostr, fmt, local );
}


} // namespace Time
