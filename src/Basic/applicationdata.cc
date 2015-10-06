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

ApplicationData::ApplicationData()
{
    if ( !QCoreApplication::instance() )
    {
	int argc = GetArgC();
	application_ = new mQtclass(QCoreApplication)(argc, GetArgV() );
    }
}


ApplicationData::~ApplicationData()
{
}


int ApplicationData::exec()
{
    return QCoreApplication::exec();
}


void ApplicationData::exit( int retcode )
{
    QCoreApplication::exit( retcode );
}


bool ApplicationData::isCommandAndCTRLSwapped()
{
    return !QCoreApplication::testAttribute( Qt::AA_MacDontSwapCtrlAndMeta );
}


void ApplicationData::swapCommandAndCTRL( bool yn )
{
    QCoreApplication::setAttribute( Qt::AA_MacDontSwapCtrlAndMeta, !yn );
}


void ApplicationData::setOrganizationName( const char* nm )
{ QCoreApplication::setOrganizationName( nm ); }


void ApplicationData::setOrganizationDomain( const char* domain )
{ QCoreApplication::setOrganizationDomain( domain ); }


void ApplicationData::setApplicationName( const char* nm )
{ QCoreApplication::setApplicationName( nm ); }


#else //No QT
ApplicationData::ApplicationData()
{ }


ApplicationData::~ApplicationData()
{
}


int ApplicationData::exec()
{ return 1; }


void ApplicationData::exit( int retcode )
{ }

void ApplicationData::setOrganizationName( const char* nm )
{ }


void ApplicationData::setOrganizationDomain( const char* domain )
{ }


void ApplicationData::setApplicationName( const char* nm )
{ }

#endif
