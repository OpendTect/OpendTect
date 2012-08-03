/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID mUnusedVar = "$Id: helpview.cc,v 1.51 2012-08-03 13:01:35 cvskris Exp $";

#include "helpview.h"

#include "envvars.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "multiid.h"
#include "oddirs.h"
#include "odinst.h"
#include "strmprov.h"
#include "ascstream.h"
#include "staticstring.h"
#include "iopar.h"


static const char* sMainIndex = "MainIndex";
static const char* sODDir = "base";
static const char* sIndexHtml = "index.html";
static const char* sBookHtml = "book1.htm";
static const char* sNotFoundHtml = "notfound.html";
static const char* sNotInstHtml = "docnotinst.html";
static const char* sToDoHtml = "todo.html";
static const char* sWebSite = "http://opendtect.org";

static bool showhelpstuff = GetEnvVarYN("DTECT_SHOW_HELP")
			 || GetEnvVarYN("DTECT_SHOW_HELPINFO_ONLY");

static StreamData getStreamData( const char* fnm )
{
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() )
    {
	BufferString msg( "Help file '" );
	msg += fnm; msg += "' not available";
	ErrMsg( msg ); sd.close();
    }
    return sd;
}


static const char* unScoped( const char* inp )
{
    if ( !inp || !*inp ) return inp;

    const char* ptr = strchr( inp, ':' );
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
    StreamData sd = getStreamData( fnm );
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
    if ( showhelpstuff )
    {
	BufferString msg( "Window ID ", inpwinid, " -> Link name '" );
	msg += ptr; msg += "'"; UsrMsg( msg );
    }

    sd.close();
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
    StreamData sd = getStreamData( ftnm );
    BufferString htmlfnm;
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
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	return false;

    ascistream astrm( *sd.istrm, true );
    iop.getFrom( astrm );
    sd.close();
    return !iop.isEmpty();
}


const char* HelpViewer::getCreditsSpecificFileName( const char* winid )
{
    IOPar iop;
    const BufferString cfnm( getCreditsFileName(winid) );
    if ( !getCreditsData(cfnm,iop) )
	return 0;
    
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
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
    static TextTranslateMgr* trmgr = 0;
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}
