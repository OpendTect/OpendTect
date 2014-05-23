/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uihelpview.h"

#include "iopar.h"
#include "oddirs.h"
#include "filepath.h"
#include "file.h"
#include "uidesktopservices.h"
#include "odversion.h"
#include "odnetworkaccess.h"
#include "keystrs.h"
#include "uimsg.h"

static IOPar urltable;


void FlareHelpProvider::initClass( const char* factorykey, const char* baseurl )
{
    urltable.set( factorykey, baseurl );
    HelpProvider::factory().addCreator(FlareHelpProvider::createInstance,
				       factorykey );
}


FlareHelpProvider::FlareHelpProvider( const char* url )
    : baseurl_( url )
{
}


void FlareHelpProvider::provideHelp( const char* argument ) const
{
    BufferString url( baseurl_ );

    if ( argument && *argument )
	url.add( "#cshid=" ).add ( argument );

    uiDesktopServices::openUrl( url );
}


HelpProvider* FlareHelpProvider::createInstance()
{
    const char* name = factory().currentName();
    return new FlareHelpProvider( urltable.find( name ) );
}

#define mHtmlFileName	"Default.htm"
#define mBaseUrl	"http://www.opendtect.org/"

static const char* fileprot = "file://";

static void initHelpSystem( const char* context )
{
    FilePath subpath( context, mHtmlFileName );
    FilePath basefile = mGetUserDocDir();
    basefile.add( subpath.fullPath() );

    BufferString url;

    if ( File::exists(basefile.fullPath() ) )
    {
	url = fileprot;
	url += basefile.fullPath(FilePath::Unix,true);
    }
    else
    {
	url = mBaseUrl;
	url.add( toString(mODVersion) );
	url.add( "/doc/User/" ).add( subpath.fullPath( FilePath::Unix ) );
    }

    FlareHelpProvider::initClass( context, url.str() );
}


void FlareHelpProvider::initODHelp()
{
    initHelpSystem( "od" );
    initHelpSystem( "appman" );
}


void DevDocHelp::initClass()
{
    HelpProvider::factory().addCreator( createInstance,
					sKeyFactoryName() );
}


BufferString DevDocHelp::getUrl() const
{
    FilePath basefile = mGetProgrammerDocDir();
    basefile.add( "index.html" );

    if ( File::exists( basefile.fullPath()) )
	return BufferString( fileprot, basefile.fullPath(FilePath::Unix) );

    BufferString url = "http://www.opendtect.org/";
    url.add( toString(mODVersion) );
    url.add( "/doc/Programmer/index.html" );

    uiString networkmsg;
    mDefineStaticLocalObject( bool, isonline, = Network::ping(url,networkmsg) );

    if ( !isonline )
	return BufferString( sKey::EmptyString() );

    return url;
}


void DevDocHelp::provideHelp( const char* arg ) const
{
    BufferString url = getUrl();

    if ( url.isEmpty() )
	uiMSG().error( "Cannot open developer's documentation" );
    else
	uiDesktopServices::openUrl( url );
}


bool DevDocHelp::hasHelp( const char* arg ) const
{
    return !getUrl().isEmpty();
}


HelpProvider* DevDocHelp::createInstance()
{
    return new DevDocHelp;
};
