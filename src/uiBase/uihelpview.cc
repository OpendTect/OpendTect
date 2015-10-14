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
#include "odinst.h"
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

static const char* fileprot = "file:///";

void FlareHelpProvider::initHelpSystem( const char* context, const char* path )
{
    FilePath subpath( path, mHtmlFileName );
    FilePath basefile = GetDocFileDir( "" );
    basefile.add( subpath.fullPath() );

    BufferString url;
    if ( File::exists(basefile.fullPath()) )
    {
	url = fileprot;
	url += basefile.fullPath();
    }
    else
    {
	url = mBaseUrl;
	url.add( toString(mODVersion) );
	url.add( "/doc/" ).add( subpath.fullPath(FilePath::Unix) );
    }

    FlareHelpProvider::initClass( context, url.str() );
}


void FlareHelpProvider::initODHelp()
{
    initHelpSystem( "od", "od_userdoc" );
    initHelpSystem( "wf", "workflows" );
    //initHelpSystem( "appman", "appmandoc" );
}


void DevDocHelp::initClass()
{
    HelpProvider::factory().addCreator( createInstance, sKeyFactoryName() );
}


BufferString DevDocHelp::getUrl() const
{
    const BufferString classpkgver( ODInst::getPkgVersion("classdoc") );
    const bool foundclspkg = !classpkgver.isEmpty() &&
			     !classpkgver.find( "error" );
    if ( !foundclspkg )
	return BufferString( sKey::EmptyString() );

    FilePath basefile = mGetProgrammerDocDir();
    basefile.add( "index.html" );
    if ( !File::exists( basefile.fullPath()) )
	return BufferString( sKey::EmptyString() );

    return BufferString( fileprot, basefile.fullPath(FilePath::Unix) );
}


void DevDocHelp::provideHelp( const char* arg ) const
{
    BufferString url = getUrl();

    if ( url.isEmpty() )
	uiMSG().error(tr("Cannot open developer's documentation"));
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

