/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-5-1994
 * FUNCTION : Functions for time
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "timefun.h"
#include "bufstring.h"
#include "perthreadrepos.h"

#ifndef OD_NO_QT
# include <QDateTime>
# include <QTime>
#endif

mUseQtnamespace

namespace Time
{

Counter::Counter()
#ifndef OD_NO_QT
    : qtime_(new QTime)
#endif
{}

Counter::~Counter()
{
#ifndef OD_NO_QT
    delete qtime_;
#endif
}

void Counter::start()
{
#ifndef OD_NO_QT
    qtime_->start();
#endif
}

int Counter::restart()
{
#ifndef OD_NO_QT
    return qtime_->restart();
#else
    return mUdf(int);
#endif
}

int Counter::elapsed() const
{
#ifndef OD_NO_QT
    return qtime_->elapsed();
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


const char* defDateTimeFmt()	{ return "ddd dd MMM yyyy, hh:mm:ss"; }
const char* defDateFmt()	{ return "ddd dd MMM yyyy"; }
const char* defTimeFmt()	{ return "hh:mm:ss"; }

const char* getDateTimeString( const char* fmt, bool local )
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    QDateTime qdt = QDateTime::currentDateTime();
    if ( !local ) qdt = qdt.toUTC();
    ret = qdt.toString( fmt );
#endif
    return ret.buf();
}

const char* getDateString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

const char* getTimeString( const char* fmt, bool local )
{ return getDateTimeString( fmt, local ); }

bool isEarlier(const char* first, const char* second, const char* fmt )
{
#ifndef OD_NO_QT
    QString fmtstr( fmt );
    QDateTime qdt1 = QDateTime::fromString( first, fmtstr );
    QDateTime qdt2 = QDateTime::fromString( second, fmtstr );
    return qdt1 < qdt2;
#else
    return false;
#endif
}

} // namespace Time
