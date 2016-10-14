/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
________________________________________________________________________

-*/

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


SimpleHelpProvider::SimpleHelpProvider( const char* baseurl, const char* fnm )
    : baseurl_(baseurl)
{
    if ( fnm && *fnm )
	keylinks_.read( fnm, 0 );
}


void SimpleHelpProvider::addKeyLink( const char* key, const char* lnk )
{
    keylinks_.set( key, lnk );
}


bool SimpleHelpProvider::hasHelp( const char* key ) const
{
    return keylinks_.hasKey( key );
}


void SimpleHelpProvider::provideHelp( const char* key ) const
{
    BufferString link;
    if ( !keylinks_.get(key,link) )
	return;

    BufferString url( baseurl_ );
    url.add( link );
    uiDesktopServices::openUrl( url );
}


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
#define mBaseUrl	"http://backend.opendtect.org/"

static const char* fileprot = "file:///";

void FlareHelpProvider::initHelpSystem( const char* context, const char* path )
{
    File::Path subpath( path, mHtmlFileName );
    File::Path basefile = GetDocFileDir( "" );
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
	url.add( "/backendscripts/docsites.php?version=" );
	url.add( toString(mODVersion) );
	url.add( "&module=" );
	url.add( path );
    }

    FlareHelpProvider::initClass( context, url.str() );
}


void FlareHelpProvider::initODHelp()
{
    initHelpSystem( "od", "od_userdoc" );
    initHelpSystem( "wf", "HTML_WF" );
    initHelpSystem( "tm", "HTML_TM" );
    //initHelpSystem( "appman", "appmandoc" );
}


// DevDocHelp
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

    File::Path basefile = mGetProgrammerDocDir();
    basefile.add( "index.html" );
    if ( !File::exists( basefile.fullPath()) )
	return BufferString( sKey::EmptyString() );

    return BufferString( fileprot, basefile.fullPath(File::Path::Unix) );
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
{ return !getUrl().isEmpty(); }


HelpProvider* DevDocHelp::createInstance()
{ return new DevDocHelp; }



// WebsiteHelp
void WebsiteHelp::initClass()
{ HelpProvider::factory().addCreator( createInstance, sKeyFactoryName() ); }

HelpProvider* WebsiteHelp::createInstance()
{ return new WebsiteHelp; }

const char* WebsiteHelp::sKeyFactoryName()	{ return "website"; }
const char* WebsiteHelp::sKeySupport()		{ return "support"; }
const char* WebsiteHelp::sKeyAttribMatrix()	{ return "attribmatrix"; }

void WebsiteHelp::provideHelp( const char* arg ) const
{
    const FixedString argstr = arg;
    BufferString url;
    if ( argstr == sKeyAttribMatrix() )
	url = "https://opendtect.org/opendtect-attributes-matrix";
    else if ( argstr == sKeySupport() )
	url = "https://opendtect.org/index.php/support";

    if ( url.isEmpty() )
	uiMSG().error( tr("Cannot open website page") );
    else
	uiDesktopServices::openUrl( url );
}


bool WebsiteHelp::hasHelp( const char* arg ) const
{
    const FixedString argstr = arg;
    return  argstr == sKeyAttribMatrix() ||
	    argstr == sKeySupport();
}

