/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "applicationdata.h"

#include "debug.h"
#include "genc.h"
#include "ptrman.h"

#ifndef OD_NO_QT
#include <QCoreApplication>

class QEventLoopReceiver : public QObject
{
public:
    QEventLoopReceiver( ApplicationData& ad )
	: ad_( ad )
    {}

    bool event( QEvent* )
    {
	Threads::MutexLocker locker( ad_.eventloopqueuelock_ );
	RefMan<CallBackSet> mycopy = new CallBackSet( ad_.eventloopqueue_ );
	ad_.eventloopqueue_.erase();

	locker.unLock();

	mycopy->doCall( &ad_ );
	return true;
    }

    ApplicationData&		ad_;
};


ApplicationData::ApplicationData()
    : eventloopreceiver_( 0 )
    , eventloopqueue_(*new CallBackSet)
{
    eventloopqueue_.ref();
    if ( !QCoreApplication::instance() )
    {
	int argc = GetArgC();
	application_ = new mQtclass(QCoreApplication)(argc, GetArgV() );
    }
}


ApplicationData::~ApplicationData()
{
    eventloopqueue_.unRef();
    deleteAndZeroPtr( eventloopreceiver_ );
    deleteAndZeroPtr( application_ );
}


int ApplicationData::exec()
{
    return QCoreApplication::exec();
}


void ApplicationData::exit( int retcode )
{
    QCoreApplication::exit( retcode );
}


void ApplicationData::addToEventLoop( const CallBack& cb )
{
    Threads::MutexLocker locker( eventloopqueuelock_ );

    if ( !eventloopqueue_.size() )
    {
	if ( !eventloopreceiver_ )
	    eventloopreceiver_ = new QEventLoopReceiver( *this );

	QCoreApplication::postEvent( eventloopreceiver_,
				     new QEvent(QEvent::None) );
    }
    eventloopqueue_ += cb;
}


void ApplicationData::setOrganizationName( const char* nm )
{ QCoreApplication::setOrganizationName( nm ); }


void ApplicationData::setOrganizationDomain( const char* domain )
{ QCoreApplication::setOrganizationDomain( domain ); }


void ApplicationData::setApplicationName( const char* nm )
{ QCoreApplication::setApplicationName( nm ); }


#else //No QT
ApplicationData::ApplicationData()
    : eventloopreceiver_( 0 )
    , eventloopqueue_(*new CallBackSet)
{
    eventloopqueue_.ref();
}


ApplicationData::~ApplicationData()
{
    eventloopqueue_.unRef();
}


int ApplicationData::exec()
{ return 1; }


void ApplicationData::exit( int retcode )
{ }

void ApplicationData::addToEventLoop( const CallBack& cb )
{ }


void ApplicationData::setOrganizationName( const char* nm )
{ }


void ApplicationData::setOrganizationDomain( const char* domain )
{ }


void ApplicationData::setApplicationName( const char* nm )
{ }

#endif

