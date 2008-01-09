/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.32 2008-01-09 13:54:34 cvsbert Exp $";

#include "helpview.h"
#include "ascstream.h"
#include "multiid.h"
#include "errh.h"
#include "oddirs.h"
#include "envvars.h"
#include "strmprov.h"
#include "filegen.h"
#include "filepath.h"
#include "string2.h"
#include <stdlib.h>

#include "debugmasks.h"

#ifdef __win__
# include <windows.h>
# include <ctype.h>


static bool GetBrowser( BufferString& defaultBrowser, BufferString& browserArgs)
{
    defaultBrowser = "";
    browserArgs = "";

    #define BSIZE 256
    static char vbuff[BSIZE];
    static char arg[32];
    UCHAR  *value = (UCHAR *)vbuff;
    UCHAR  name[BSIZE];
    HKEY  mykey;
    DWORD  keyNameSize;
    DWORD  keyValSize;
    LONG  ret;
    char  *keyName = "HTTP\\shell\\open\\command";
    DWORD  keyType;


    ret = RegOpenKeyEx( HKEY_CLASSES_ROOT, keyName, 0,
			KEY_ALL_ACCESS, &mykey);

    if (ret != ERROR_SUCCESS)
    {
	char* keynm = "http\\shell\\open\\command";

	ret = RegOpenKeyEx( HKEY_CLASSES_ROOT, keynm, 0,
			    KEY_ALL_ACCESS, &mykey);

	if (ret != ERROR_SUCCESS)
	{
	    defaultBrowser =
			"C:\\Program Files\\Internet Explorer\\iexplore.exe";
	    browserArgs = " -nohome ";


	    if ( DBG::isOn(DBG_DBG) )
	    {
		BufferString
		    msg( "Unable to find browser in registry. Trying: " );
		msg += defaultBrowser;
		msg += " ";
		msg += browserArgs;

		DBG::message( msg );
	    }

	    return false;
	}
    }

    memset(value, 0, sizeof(value));

    for (int ii = 0; ; ii++)
    {
	keyValSize = BSIZE;
	keyNameSize = BSIZE;
	ret = RegEnumValue( mykey, ii, (char *)name, &keyNameSize, NULL,
			    &keyType, (UCHAR *)value, &keyValSize);

	if ( ret != ERROR_SUCCESS )
	    break;

	if ( *name )         // If this isn't the default
	    continue;        //    Then don't use it

	char* chr = (char *)value;

	// Replace all quotes with spaces
	while (1)
	{
	    if ((chr = strchr(chr, '"')) == NULL)
		break;
	    *chr++ = ' ';
	}


	// Remove everything from '%' on
	if ((chr = strchr((LPCSTR)value, '%')) != NULL)
	    *chr = '\0';       //


	// Move all chars after '.exe' to argument buffers

	chr = strstr((LPCSTR)value, ".exe");
	if (!chr)
	    chr = strstr((LPCSTR)value, ".EXE");
	if (!chr)         // No '.exe' string in file name
	    break;

	chr += 4;         // Bump past ".exe"
	if (*chr)         // If any command tail
	{
	    strcpy(arg, chr);      // Copy command tail to argument buffer
	    *chr = '\0';       // NULL terminate command
	    browserArgs = (char *)arg;    // Save arguments
	}
	if (*value == (UCHAR)' ')
	    value++;

	// Get --> default browser exe file
	defaultBrowser = (const char *)value;
    }

    RegCloseKey(mykey);

    return ( !defaultBrowser.isEmpty() );
}

#endif

static const char* sMainIndex = "MainIndex";
static const char* sNotInstHtml = "docnotinst.html";
BufferString HelpViewer::applnm( "dTect" );


const char* HelpViewer::subdirNm( const char* scnm )
{
    static char ret[40];
    strcpy( ret, scnm && *scnm ? scnm : applnm.buf() );
    strcat( ret, "Doc" );
    return ret;
}


void HelpViewer::use( const char* url, const char* wintitl )
{
    static BufferString _url;
    _url = url;
    if ( !url || !*url )
    {
	_url = getURLForLinkName( sMainIndex, applnm );
    }


#ifdef __win__

    BufferString browser;
    BufferString args;
    GetBrowser( browser, args );

    args += " \"file://";
    replaceCharacter( _url.buf(), '\\' , '/' );
    args += _url;
    args += "\"";

    if ( DBG::isOn(DBG_DBG) )
    {
        BufferString msg( "Launching browser: " );
	msg += browser;
	msg += " ";
	msg += args;

        DBG::message( msg );
    }


    HINSTANCE ret = ShellExecute( NULL, NULL, browser, args, NULL, SW_NORMAL);
    int err = (int) ret;

    if ( err > 32 ) return;

    char *ptr = NULL;
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	FORMAT_MESSAGE_FROM_SYSTEM,
	0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

    BufferString errmsg( "Error opening \"" );
    errmsg += url;
    errmsg += "\" :\n";
    errmsg += ptr;

    ErrMsg( errmsg );

    LocalFree(ptr);

#else

    replaceCharacter(_url.buf(),' ','%');
    url = (const char*)_url;

    BufferString cmd( "@" );
    cmd += mGetExecScript();

    cmd += " HtmlViewer \"";
    cmd += url; cmd += "\" ";
    BufferString title( wintitl );
    if ( title.isEmpty() )
	title = "Documentation for OpendTect";
    cleanupString( title.buf(), NO, YES, YES );
    cmd += title;

    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand(false) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
    }

#endif
}


#define mGetDataFileName(fnm) GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm)

void HelpViewer::doHelp( const char* relurl, const char* wt )
{
    BufferString docpath( mGetDataFileName(relurl) );
    BufferString wintitle( wt );
    replaceCharacter( wintitle.buf(), ' ', '_' );
    use( docpath, wintitle );
}


static StreamData openHFile( const char* nm, const char* scope )
{
    FileNameString fnm;
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(mGetDataFileName(subfnm)) )
	fnm = mGetDataFileName( sNotInstHtml );
    else
    {
	subfnm = FilePath( subfnm ).add( nm ).fullPath();
	fnm = mGetDataFileName( subfnm );
    }

    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() )
    {
	FileNameString msg( "Help file '" );
	msg += fnm; msg += "' not available";
	ErrMsg( msg ); sd.close();
    }
    return sd;
}


static const char* unScoped( const char* inp )
{
    if ( !inp || !*inp ) return inp;

    char* ptr = strchr( inp, ':' );
    return ptr ? ptr + 1 : inp;
}


static BufferString getScope( const char* inp )
{
    BufferString ret;
    BufferString winid( inp );
    char* ptr = strchr( winid.buf(), ':' );
    if ( ptr )
    {
	*ptr = '\0';
	ret = winid;
    }
    return ret;
}


BufferString HelpViewer::getLinkNameForWinID( const char* inpwinid )
{
    if ( !inpwinid || !*inpwinid ) return BufferString( sMainIndex );

    BufferString scope( getScope(inpwinid) );
    BufferString winid( unScoped(inpwinid) );

    StreamData sd = openHFile( "WindowLinkTable.txt", scope );
    if ( !sd.usable() )
	return BufferString("");
    std::istream& strm = *sd.istrm;

    ascistream astream( strm );
    MultiID code[3];
    MultiID wid;
    int lvl;
    const char* ptr = 0;
    while ( 1 )
    {
	char c = strm.peek();
	while ( c == '\n' ) { strm.ignore( 1 ); c = strm.peek(); }
	lvl = 0;
	if ( c == '\t' )
	{
	    lvl++;
	    strm.ignore( 1 );
	    c = strm.peek();
	    if ( c == '\t' ) lvl++;
	}
	astream.next();
	if ( atEndOfSection(astream) ) break;

	code[lvl] = astream.keyWord();
	for ( int idx=2; idx>lvl; idx-- )
	    code[idx] = "";
	wid = code[0];
	if ( !code[1].isEmpty() )
	{
	    wid += code[1];
	    if ( !code[2].isEmpty() )
		wid += code[2];
	}
	if ( wid != winid.buf() ) continue;

	ptr = astream.value();
	mSkipBlanks(ptr);

	// Skip object name
	mSkipNonBlanks( ptr );
	if ( ! *ptr ) { strm.ignore(10000,'\n'); ptr = 0; continue; }
	mSkipBlanks(ptr);

	const char* endptr = ptr;
	mSkipNonBlanks( endptr );
	*(char*)endptr = '\0';
	break;
    }

    if ( !ptr || ! *ptr )
    {
	BufferString msg = "No specific help for this window (ID=";
	msg += inpwinid; msg += ").";
	UsrMsg( msg );
	ptr = sMainIndex;
    }
    else if ( GetEnvVarYN("DTECT_SHOW_HELP") )
    {
	BufferString msg = inpwinid; msg += " -> "; msg += ptr;
	UsrMsg( msg );
    }

    sd.close();
    return BufferString( ptr );
}


BufferString HelpViewer::getURLForLinkName( const char* lnm, const char* scope )
{
    BufferString linknm( lnm );
    if ( linknm.isEmpty() )
	linknm = sMainIndex;
    BufferString htmlfnm;
    const bool ismainidx = linknm == sMainIndex;
    const bool istodo = linknm == "TODO";
    if ( !ismainidx && !istodo )
    {
	StreamData sd = openHFile( "LinkFileTable.txt", scope );
	if ( sd.usable() )
	{
	    std::string lnk, fnm;
	    std::istream& strm = *sd.istrm;
	    while ( strm.good() )
	    {
		strm >> lnk >> fnm;
		if ( caseInsensitiveEqual(lnk.c_str(),linknm.buf(),0) )
		{
		    htmlfnm = fnm.c_str();
		    linknm = lnk.c_str();
		    break;
		}
	    }
	    sd.close();
	}
    }

    FilePath fp( GetSoftwareDir() );
    fp.add( "data" ).add( subdirNm(scope) );
    if ( istodo )
	htmlfnm = "todo.html";
    else if ( htmlfnm.isEmpty() )
	htmlfnm = "index.html";

    fp.add( htmlfnm );
    BufferString url = fp.fullPath();
    if ( ismainidx && !File_exists(url) )
    {
	fp.setFileName( htmlfnm == "index.html" ? "book1.htm" : "index.html" );
	url = fp.fullPath();
    }

    if ( !File_exists(url) )
	url = mGetDataFileName( istodo ? "todo.html" : "notfound.html" );
    else if ( linknm != sMainIndex && !istodo )
	{ url += "#"; url += linknm; }

    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    BufferString scope( getScope(winid) );
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(mGetDataFileName(subfnm)) )
	return BufferString( mGetDataFileName(sNotInstHtml) );

    BufferString lnm = getLinkNameForWinID( winid );
    if ( lnm.isEmpty() ) return lnm;
    return getURLForLinkName( lnm, scope );
}
