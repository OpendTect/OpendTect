/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.21 2004-02-02 11:25:44 dgb Exp $";

#include "helpview.h"
#include "ascstream.h"
#include "multiid.h"
#include "errh.h"
#include "strmprov.h"
#include "filegen.h"
#include "string2.h"
#include <stdlib.h>

#ifdef __win__
# include <windows.h>
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

    HINSTANCE ret = ShellExecute(NULL,"open",_url,NULL,NULL,SW_NORMAL);
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
    if ( wintitl )
	cmd += wintitl;
    else
	{ cmd += applnm; cmd += "_Documentation"; }
    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand(false) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
    }

#endif
}


void HelpViewer::doHelp( const char* relurl, const char* wt )
{
    BufferString docpath( GetDataFileName(relurl) );
    BufferString wintitle( wt );
    replaceCharacter( wintitle.buf(), ' ', '_' );
    use( docpath, wintitle );
}


static StreamData openHFile( const char* nm, const char* scope )
{
    FileNameString fnm;
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(GetDataFileName(subfnm)) )
	fnm = GetDataFileName( sNotInstHtml );
    else
    {
	subfnm = File_getFullPath( subfnm, nm );
	fnm = GetDataFileName( subfnm );
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
    istream& strm = *sd.istrm;

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
	if ( code[1] != "" )
	{
	    wid += code[1];
	    if ( code[2] != "" )
		wid += code[2];
	}
	if ( wid != winid.buf() ) continue;

	ptr = astream.value();
	skipLeadingBlanks(ptr);

	// Skip object name
	while ( *ptr && !isspace(*ptr) ) ptr++;
	if ( ! *ptr ) { strm.ignore(10000,'\n'); ptr = 0; continue; }
	skipLeadingBlanks(ptr);

	const char* endptr = ptr;
	while ( *endptr && !isspace(*endptr) ) endptr++;
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
    else if ( getenv("DTECT_SHOW_HELP") )
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
    if ( linknm == "" )
	linknm = sMainIndex;
    BufferString htmlfnm;
    if ( linknm != sMainIndex )
    {
	StreamData sd = openHFile( "LinkFileTable.txt", scope );
	if ( sd.usable() )
	{
	    string lnk, fnm;
	    istream& strm = *sd.istrm;
	    while ( strm.good() )
	    {
		strm >> lnk >> fnm;
		if ( caseInsensitiveEqual(lnk.c_str(),(const char*)linknm,0) )
		{
		    htmlfnm = fnm.c_str();
		    linknm = lnk.c_str();
		    break;
		}
	    }
	    sd.close();
	}
    }

    BufferString url( GetSoftwareDir() );
    url = File_getFullPath( url, "data" );
    url = File_getFullPath( url, subdirNm(scope) );

    if ( htmlfnm == "" )
	htmlfnm = "index.html";
    url = File_getFullPath( url, htmlfnm );
    if ( !File_exists(url) )
    {
	url = File_getPathOnly( url );
	if ( htmlfnm == "index.html" )
	    htmlfnm = "book1.htm";
	else
	    htmlfnm = "index.html";
	url = File_getFullPath( url, htmlfnm );
    }
    if ( !File_exists(url) )
	url = GetDataFileName( "notfound.html" );
    else if ( linknm != sMainIndex )
	{ url += "#"; url += linknm; }

    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    BufferString scope( getScope(winid) );
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(GetDataFileName(subfnm)) )
	return BufferString( GetDataFileName(sNotInstHtml) );

    BufferString lnm = getLinkNameForWinID( winid );
    if ( lnm == "" ) return lnm;
    return getURLForLinkName( lnm, scope );
}
