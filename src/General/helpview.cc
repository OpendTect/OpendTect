/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.6 2003-01-13 17:06:50 bert Exp $";

#include "helpview.h"
#include "ascstream.h"
#include "multiid.h"
#include "errh.h"
#include "strmprov.h"
#include "filegen.h"
#include <stdlib.h>


static const char* sMainIndex = "MainIndex";

static const char* subdirNm()
{
    return GetDgbApplicationCode() == mDgbApplCodeDTECT ? "dTectDoc" : "GDIDoc";
}


void HelpViewer::use( const char* url )
{
    if ( !url || !*url )
    {
	static BufferString tmp;
	tmp = getURLForLinkName( sMainIndex );
	url = (const char*)tmp;
    }

    BufferString cmd( "@" );
    cmd += GetSoftwareDir();
    cmd = File_getFullPath( cmd, "bin" );
    cmd = File_getFullPath( cmd, "dgb_exec" );
    cmd += " HtmlViewer \"";
    cmd += url; cmd += "\" ";
    cmd += GetDgbApplicationCode() == mDgbApplCodeDTECT
	? "dTect_Help" : "GDI_Help";
    StreamProvider strmprov( cmd );
    if ( !strmprov.executeCommand(false) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
    }
}


static StreamData openHFile( const char* nm )
{
    BufferString subfnm( subdirNm() );
    subfnm = File_getFullPath( subfnm, nm );
    FileNameString fnm = GetDataFileName( subfnm );

    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() )
    {
	FileNameString msg( "Help file '" );
	msg += fnm; msg += "' not available";
	ErrMsg( msg ); sd.close();
    }
    return sd;
}


BufferString HelpViewer::getLinkNameForWinID( const char* winid )
{
    StreamData sd = openHFile( "WindowLinkTable.txt" );
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
	if ( wid != winid ) continue;

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
	msg += winid; msg += ").";
	UsrMsg( msg );
	ptr = sMainIndex;
    }
    else if ( getenv("dGB_SHOW_HELP") )
    {
	BufferString msg = winid; msg += " -> "; msg += ptr;
	UsrMsg( msg );
    }

    sd.close();
    return BufferString( ptr );
}


BufferString HelpViewer::getURLForLinkName( const char* lnm )
{
    BufferString linknm( lnm );
    if ( linknm == "" )
	linknm = sMainIndex;
    BufferString htmlfnm;
    if ( linknm != sMainIndex )
    {
	StreamData sd = openHFile( "LinkFileTable.txt" );
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

    if ( htmlfnm == "" )
	htmlfnm = "index.html";

    BufferString url( GetSoftwareDir() );
    url = File_getFullPath( url, "data" );
    url = File_getFullPath( url, subdirNm() );
    url = File_getFullPath( url, htmlfnm );
    if ( linknm != sMainIndex )
	{ url += "#"; url += linknm; }
    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    BufferString lnm = getLinkNameForWinID( winid );
    if ( lnm == "" ) return lnm;
    return getURLForLinkName( lnm );
}
