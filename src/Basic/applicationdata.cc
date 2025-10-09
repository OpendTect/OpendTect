/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"

#include "debug.h"
#include "genc.h"
#include "odruncontext.h"
#include "perthreadrepos.h"
#include "ptrman.h"

#include <QCoreApplication>
#include <iostream>


ApplicationData::ApplicationData()
{
    if ( !hasInstance() )
    {
	int argc = GetArgC();
	application_ = new mQtclass(QCoreApplication)(argc, GetArgV() );
    }
}


ApplicationData::~ApplicationData()
{
    delete application_;
}


bool ApplicationData::hasInstance()
{
    return QCoreApplication::instance();
}


Notifier<ApplicationData>& ApplicationData::applicationToBeStarted()
{
    static PtrMan<Notifier<ApplicationData> > thenotif_;
    if ( !thenotif_ )
	thenotif_ = new Notifier<ApplicationData>( nullptr );

    return *thenotif_.ptr();
}


int ApplicationData::exec()
{
    applicationToBeStarted().trigger();
    return QCoreApplication::exec();
}


void ApplicationData::exit( int retcode )
{
    if ( !hasInstance() )
	DBG::forceCrash(false);

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
{
    QCoreApplication::setOrganizationName( nm );
}


void ApplicationData::setOrganizationDomain( const char* domain )
{
    QCoreApplication::setOrganizationDomain( domain );
}


void ApplicationData::setApplicationName( const char* nm )
{
    QCoreApplication::setApplicationName( nm );
}


BufferString ApplicationData::organizationName()
{
    const QString qstr = QCoreApplication::organizationName();
    return qstr;
}


BufferString ApplicationData::organizationDomain()
{
    const QString qstr = QCoreApplication::organizationDomain();
    return qstr;
}


BufferString ApplicationData::applicationName()
{
    const QString qstr = QCoreApplication::applicationName();
    return qstr;
}


const char* ApplicationData::sDefOrganizationName()
{
    return "dGB Earth Sciences";
}


const char* ApplicationData::sDefOrganizationDomain()
{
    return "opendtect.org";
}


const char* ApplicationData::sDefApplicationName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !AreProgramArgsSet() )
	{
	    std::cerr << "(PE) SetProgramArgs not called in time" << std::endl;
	    return nullptr;
	}

	 BufferString applnm( GetExecutableName() );
	 if ( HasDebugPostFix() )
	 {
	     const BufferString postfix = GetDebugPostFix();
	     const char* lastocc = applnm.findLast( postfix.buf() );
	     if ( lastocc && *lastocc )
		 (char&) *lastocc = '\0';
	 }

	 ret = applnm.str();
    }

    return ret.buf();
}


void ApplicationData::sSetDefaults()
{
    setOrganizationName( sDefOrganizationName() );
    setOrganizationDomain( sDefOrganizationDomain() );
    setApplicationName( sDefApplicationName() );
}
