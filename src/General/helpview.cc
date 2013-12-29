/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "helpview.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "multiid.h"
#include "oddirs.h"
#include "odinst.h"
#include "ascstream.h"
#include "staticstring.h"
#include "msgh.h"
#include "iopar.h"


static const char* sMainIndex = "MainIndex";
static const char* sODDir = "base";
static const char* sIndexHtml = "index.html";
static const char* sBookHtml = "book1.htm";
static const char* sNotFoundHtml = "notfound.html";
static const char* sNotInstHtml = "docnotinst.html";
static const char* sToDoHtml = "todo.html";
static const char* sWebSite = "http://opendtect.org";

static bool showhelpstuff = false;



void HelpViewer::init()
{
    showhelpstuff = GetEnvVarYN("DTECT_SHOW_HELP")
		|| GetEnvVarYN("DTECT_SHOW_HELPINFO_ONLY");
}


static od_istream getIStream( const char* fnm )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	BufferString msg( "Help file '" );
	msg.add( fnm ).add( "' not available: \"" )
		.add( strm.errMsg() ).add( "\"" );
	ErrMsg( msg );
    }
    return strm;
}


static const char* unScoped( const char* inp )
{
    if ( !inp || !*inp ) return inp;

    const char* ptr = firstOcc( inp, ':' );
    return ptr ? ptr + 1 : inp;
}


static BufferString getScope( const char* inp )
{
    BufferString ret;
    BufferString winid( inp );
    char* ptr = winid.find( ':' );
    if ( ptr )
	{ *ptr = '\0'; ret = winid; }

    if ( ret.isEmpty() )
	ret = sODDir;
    return ret;
}

#define mIsInvalid(str) \
    (!str || !*str || (*str == '-' && !*(str+1)))


BufferString HelpViewer::getLinkNameForWinID( const char* inpwinid,
					      const char* docdir )
{
    if ( mIsInvalid(inpwinid) )
	return BufferString( sMainIndex );

    BufferString winid( unScoped(inpwinid) );

    const BufferString fnm =
	FilePath( docdir ).add( "WindowLinkTable.txt" ).fullPath();
    od_istream strm = getIStream( fnm );
    if ( !strm.isOK() )
	return BufferString("");

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
	if ( wid != MultiID(winid) ) continue;

	ptr = astream.value();
	mSkipBlanks(ptr);

	// Skip object name
	mSkipNonBlanks( ptr );
	if ( ! *ptr ) { strm.skipUntil('\n'); ptr = 0; continue; }
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
    if ( showhelpstuff )
    {
	BufferString msg( "Window ID ", inpwinid, " -> Link name '" );
	msg += ptr; msg += "'"; UsrMsg( msg );
    }

    return BufferString( ptr );
}


BufferString HelpViewer::getURLForLinkName( const char* lnm, const char* docdir)
{
    if ( mIsInvalid(lnm) )
	lnm = sMainIndex;

    BufferString linknm( lnm );
    const bool ismainidx = linknm == sMainIndex;
    const bool istodo = linknm == "TODO";

    BufferString url;
    if ( ismainidx || istodo )
    {
	url = ismainidx ? FilePath( docdir ).add( sIndexHtml ).fullPath().buf()
			: GetDocFileDir( sToDoHtml );
	if ( ismainidx && !File::exists(url) )
	{
	    url = FilePath( docdir ).add( sBookHtml ).fullPath();
	    if ( !File::exists(url) )
		url = GetDocFileDir( sNotFoundHtml );
	}
	return url;
    }

    const BufferString ftnm =
	FilePath( docdir ).add( "LinkFileTable.txt" ).fullPath();
    od_istream strm = getIStream(ftnm);
    BufferString htmlfnm;
    if ( strm.isOK() )
    {
	BufferString lnk, fnm;
	while ( strm.isOK() )
	{
	    strm.get( lnk ).get( fnm );
	    if ( caseInsensitiveEqual(lnk.buf(),linknm.buf(),0) )
		{ htmlfnm = fnm; linknm = lnk; break; }
	}
	strm.close();
    }

    const char* fnm = htmlfnm.isEmpty() ? sIndexHtml : htmlfnm.buf();
    FilePath urlfp( docdir ); urlfp.add( fnm );
    url = urlfp.fullPath();
    bool doesexist = File::exists(url);
    if ( !doesexist )
    {
	url = getWebUrlFromLocal( url.buf() );
	doesexist = !url.isEmpty();
    }

    if ( showhelpstuff )
    {
	BufferString msg( "Link '", linknm, "' -> URL '" );
	msg += url; msg += "'";
	if ( !doesexist )
	    { msg += " not there -> "; msg += GetDocFileDir( sNotFoundHtml ); }
	UsrMsg( msg );
    }

    if ( !doesexist )
	url = GetDocFileDir( sNotFoundHtml );
    else
	{ url += "#"; url += linknm; }
    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    const BufferString docdir =
	FilePath( mGetUserDocDir() ).add( getScope(winid) ).fullPath();
    if ( !File::exists(docdir) )
	return BufferString( GetDocFileDir(sNotInstHtml) );

    const BufferString lnm = getLinkNameForWinID( winid, docdir );
    return getURLForLinkName( lnm.buf(), docdir );
}


BufferString HelpViewer::getCreditsFileName( const char* winid )
{
    return FilePath( GetDocFileDir("Credits"), getScope(winid), "index.txt" )
		.fullPath();
}


bool HelpViewer::getCreditsData( const char* fnm, IOPar& iop )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	return false;
    strm >> iop;
    return !iop.isEmpty();
}


const char* HelpViewer::getCreditsSpecificFileName( const char* winid )
{
    IOPar iop;
    const BufferString cfnm( getCreditsFileName(winid) );
    if ( !getCreditsData(cfnm,iop) )
	return 0;

    mDeclStaticString( ret );
    ret = iop.find( unScoped(winid) );
    return ret.isEmpty() ? 0 : ret.buf();
}


BufferString HelpViewer::getCreditsURLForWinID( const char* winid )
{
    const char* fnm = getCreditsSpecificFileName( winid );
    if ( !fnm || !*fnm )
	return BufferString( GetDocFileDir(sNotFoundHtml) );

    FilePath fp( getCreditsFileName(winid) ); fp.setFileName( fnm );
    return BufferString( fp.fullPath() );
}


bool HelpViewer::hasSpecificCredits( const char* winid )
{
    return getCreditsSpecificFileName(winid);
}


BufferString HelpViewer::getWebUrlFromLocal( const char* localfnm )
{
    FilePath localfp( localfnm );
    BufferString url;
    bool docfound = false;
    FilePath fp;
    for ( int idx=0; idx<localfp.nrLevels(); idx++ )
    {
	if ( localfp.dir(idx) == "doc" )
	    docfound = true;

	if ( docfound )
	    fp.add( localfp.dir(idx).buf() );
    }

    if ( docfound )
    {
	ODInst::RelType reltype = ODInst::getRelType();
	url = sWebSite;
	url += reltype == ODInst::Development ? "/devel/" : "/rel/";
	url += fp.fullPath( FilePath::Unix );
    }

    return url;
}

#include "texttranslator.h"
mGlobal( General )  TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}
