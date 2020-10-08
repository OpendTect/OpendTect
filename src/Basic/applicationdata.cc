/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 1994
-*/

#include "applicationdata.h"

#include "debug.h"
#include "genc.h"
#include "ptrman.h"
#include "uistring.h"

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


void ApplicationData::setOrganizationName( const uiString& nm )
{
    QString qstr;
    nm.fillQString( qstr );
    QCoreApplication::setOrganizationName( qstr );
}


void ApplicationData::setOrganizationDomain( const char* domain )
{
    QCoreApplication::setOrganizationDomain( QString(domain) );
}


void ApplicationData::setApplicationName( const uiString& nm )
{
    QString qstr;
    nm.fillQString( qstr );
    QCoreApplication::setApplicationName( qstr );
}
