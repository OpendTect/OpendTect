/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


static IOPar& sUrlTable()
{
    static PtrMan<IOPar> iop = new IOPar;
    return *iop;
}


void FlareHelpProvider::initClass( const char* factorykey, const char* baseurl )
{
    sUrlTable().set( factorykey, baseurl );
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
    return new FlareHelpProvider( sUrlTable().find( name ) );
}

#define mHtmlFileName	"Default.htm"
#define mBaseUrl	"http://backend.opendtect.org/"

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
#ifdef __win__
	url.replace( '\\', '/' );
#endif
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
    initHelpSystem( "admin", "admindoc" );
    initHelpSystem( "dev", "Programmer" );
}


// WebsiteHelp
void WebsiteHelp::initClass()
{ HelpProvider::factory().addCreator( createInstance, sKeyFactoryName() ); }

HelpProvider* WebsiteHelp::createInstance()
{ return new WebsiteHelp; }

const char* WebsiteHelp::sKeyFactoryName()	{ return "website"; }
const char* WebsiteHelp::sKeySupport()		{ return "support"; }
const char* WebsiteHelp::sKeyVideos()		{ return "videos"; }
const char* WebsiteHelp::sKeyAttribMatrix()	{ return "attribmatrix"; }
const char* WebsiteHelp::sKeyFreeProjects()	{ return "freeprojects"; }
const char* WebsiteHelp::sKeyCommProjects()	{ return "commprojects"; }

void WebsiteHelp::provideHelp( const char* arg ) const
{
    const StringView argstr = arg;
    BufferString url;
    if ( argstr == sKeyAttribMatrix() )
	url = "https://www.dgbes.com/index.php/software/attributes-table";
    else if ( argstr == sKeySupport() )
	url = "https://dgbes.com/index.php/support";
    else if ( argstr == sKeyVideos() )
	url = "https://videos.opendtect.org";
    else if ( argstr == sKeyFreeProjects() )
	url = "https://terranubis.com/datalist/free";
    else if ( argstr == sKeyCommProjects() )
	url = "https://terranubis.com/datalist";

    if ( url.isEmpty() )
	uiMSG().error( tr("Cannot open website page") );
    else
	uiDesktopServices::openUrl( url );
}


bool WebsiteHelp::hasHelp( const char* arg ) const
{
    const StringView argstr = arg;
    return  argstr == sKeyAttribMatrix() ||
	    argstr == sKeySupport();
}


// VideoProvider
static IOPar& sVideoIndexFiles()
{
    static PtrMan<IOPar> iop = new IOPar;
    return *iop;
}


VideoProvider::VideoProvider( const char* idxfnm )
    : indexfilenm_(idxfnm)
{
    videolinks_.read( idxfnm, "Video definitions" );
}


void VideoProvider::init()
{
    const BufferString fnm = GetDocFileDir( "Videos.od" );
    VideoProvider::initClass( "odvideo", fnm );
}


void VideoProvider::initClass( const char* context, const char* indexfnm )
{
    HelpProvider::factory().addCreator( createInstance, context );
    sVideoIndexFiles().set( context, indexfnm );
}


HelpProvider* VideoProvider::createInstance()
{
    const char* name = factory().currentName();
    return new VideoProvider( sVideoIndexFiles().find(name) );
}


int VideoProvider::indexOf( const char* arg ) const
{
    int idx = 1;
    while ( true )
    {
	BufferString helpid;
	const bool res =
	    videolinks_.get( IOPar::compKey(toString(idx),sKey::ID()), helpid );
	if ( !res )
	    return -1;

	if ( helpid.startsWith("0x") )
	    helpid = Conv::to<int>( helpid.buf() );

	if ( helpid == arg )
	    return idx;

	idx++;
    }

    return -1;
}


bool VideoProvider::hasHelp( const char* arg ) const
{
    const int helpidx = indexOf( arg );
    return helpidx >= 0;
}


static const char* sVideoBaseUrl = "http://videos.opendtect.org/?id=";

void VideoProvider::provideHelp( const char* arg ) const
{
    const int helpidx = indexOf( arg );
    BufferString url;
    videolinks_.get( IOPar::compKey(toString(helpidx),"Url"), url );
    if ( url.isNumber(true) )
	url = BufferString( sVideoBaseUrl, url );
    if ( !url.isEmpty() )
	uiDesktopServices::openUrl( url );
}


uiString VideoProvider::description( const char* arg ) const
{
    const int helpidx = indexOf( arg );
    BufferString info;
    videolinks_.get( IOPar::compKey(toString(helpidx),"Title"), info );
    return toUiString(info);
}


//ReleaseNotesProvider
const char* ReleaseNotesProvider::sKeyFactoryName()	{ return "relinfo"; }

HelpProvider* ReleaseNotesProvider::createInstance()
{ return new ReleaseNotesProvider; }


void ReleaseNotesProvider::initClass()
{ HelpProvider::factory().addCreator( createInstance, sKeyFactoryName() ); }


void ReleaseNotesProvider::provideHelp( const char* arg ) const
{
    BufferString url;
    bool online = false;
    const StringView argstr( arg );
    if ( argstr.isEmpty() )
    {
	BufferString relnotesfnm( "Release_Notes_" );
	relnotesfnm.add( mODMajorVersion ).add( "_" ).add( mODMinorVersion );
	FilePath relnotesfp( GetSoftwareDir(true) );
#ifdef __mac__
	relnotesfp.add( "Resources" );
#endif
	relnotesfp.add( "relinfo" ).add( relnotesfnm );
	relnotesfp.setExtension( "pdf" );
	url.set( relnotesfp.fullPath() );
	if ( !File::exists(relnotesfp.fullPath()) )
	{
	    //Not found locally. Showing online relnotes.
	    arg = sKeyFactoryName();
	    online = true;
	}
    }
    else
	online = true;

    if ( online )
    {
	url.set( mBaseUrl );
	url.add( "/backendscripts/docsites.php?version=" )
	   .add( toString(mODVersion) ).add( "&module=" ).add( arg );
    }

    uiDesktopServices::openUrl( url );
}


bool ReleaseNotesProvider::hasHelp( const char* arg ) const
{
    const StringView argstr( arg );
    return  argstr == sKeyFactoryName();
}
