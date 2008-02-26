/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.33 2008-02-26 11:09:34 cvsnanne Exp $";

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


static const char* sMainIndex = "MainIndex";
static const char* sNotInstHtml = "docnotinst.html";

BufferString HelpViewer::basenm( "base" );


const char* HelpViewer::subdirNm( const char* scnm )
{
    static char ret[40];
    strcpy( ret, scnm && *scnm ? scnm : basenm.buf() );
//    strcat( ret, "Doc" );
    return ret;
}


#define mGetDataFileName(fnm) GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm)
#define mGetDocFileName(fnm) GetSetupDataFileName(ODSetupLoc_SWDirOnly,fnm)

static const char* getDocFileName( const char* fnm )
{
    static BufferString docfnm;
    docfnm = mGetUserDocDir();
    docfnm = FilePath( docfnm ).add( fnm ).fullPath();
    return docfnm.buf();
}


static StreamData openHFile( const char* nm, const char* scope )
{
    FileNameString fnm;
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(getDocFileName(subfnm)) )
	fnm = getDocFileName( sNotInstHtml );
    else
    {
	subfnm = FilePath( subfnm ).add( nm ).fullPath();
	fnm = getDocFileName( subfnm );
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
    fp.add( "doc" ).add( subdirNm(scope) );
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
	url = getDocFileName( istodo ? "todo.html" : "notfound.html" );
    else if ( linknm != sMainIndex && !istodo )
	{ url += "#"; url += linknm; }

    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    BufferString scope( getScope(winid) );
    BufferString subfnm( HelpViewer::subdirNm(scope) );
    if ( !File_exists(getDocFileName(subfnm)) )
	return BufferString( getDocFileName(sNotInstHtml) );

    BufferString lnm = getLinkNameForWinID( winid );
//    if ( lnm.isEmpty() ) return lnm;
    return getURLForLinkName( lnm, scope );
}
